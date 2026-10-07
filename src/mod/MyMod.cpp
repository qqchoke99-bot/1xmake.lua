#include "mod/MyMod.h"

#include <ll/api/mod/RegisterHelper.h>
#include <pl/memory/Hook.hpp>
#include <pl/memory/Signature.hpp>

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <string>

/*
 * Camera path copied from CameraOverhaul 1.1.2-beta (GPL-3.0 / Mirsario-derived):
 *   - SignatureId::CameraBlendSystemTick
 *   - detour void(component, blendState, factor)
 *   - write quaternion at component + 0x28 after original
 *
 * Head-bob math remains the spring-damper port from Realistic Head Bobbing (Java).
 */

namespace {

// CameraOverhaul Signatures.cpp — CameraBlendSystemTick
constexpr const char* kCameraBlendSig =
    "? ? ? D1 ? ? ? 6D ? ? ? 6D ? ? ? 6D ? ? ? 6D ? ? ? A9 ? ? ? F9 ? ? ? A9 ? ? ? A9 ? ? ? 91 55 D0 3B D5 F3 03 01 AA F4 03 00 AA ? ? ? F9 ? ? ? 91 ? ? ? 91";

// Camera transform written by CameraBlendSystem::_tick
constexpr std::uintptr_t kRotationOffset = 0x28;

constexpr float kPi     = 3.14159265358979323846f;
constexpr float kDeg2Rad = kPi / 180.0f;

struct Quat {
    float x, y, z, w;
};

float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

Quat quatMul(const Quat& a, const Quat& b) {
    return Quat{
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}

Quat axisAngle(float ax, float ay, float az, float angle) {
    const float h = angle * 0.5f;
    const float s = std::sin(h);
    return Quat{ax * s, ay * s, az * s, std::cos(h)};
}

Quat quatNormalize(const Quat& q) {
    const float n = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (!(n > 1e-8f)) return Quat{0.f, 0.f, 0.f, 1.f};
    return Quat{q.x / n, q.y / n, q.z / n, q.w / n};
}

} // namespace

RealisticHeadBobMod& RealisticHeadBobMod::instance() {
    static RealisticHeadBobMod inst;
    return inst;
}

RealisticHeadBobMod::RealisticHeadBobMod()
    : mSelf(*ll::mod::NativeMod::current()) {}

bool RealisticHeadBobMod::load() {
    auto& log = getSelf().getLogger();
    log.info("Realistic Head Bob load");

    const auto cfgPath = getSelf().getModDir() / "config" / "config.json";
    auto& cfg = headbob::Config::get();
    if (!cfg.loadFromFile(cfgPath.string())) {
        cfg.loadDefaults();
        if (cfg.cameraBlendSig.empty()) {
            cfg.cameraBlendSig = kCameraBlendSig;
        }
        std::error_code ec;
        std::filesystem::create_directories(cfgPath.parent_path(), ec);
        cfg.saveToFile(cfgPath.string());
        log.info("Wrote default config");
    } else {
        if (cfg.cameraBlendSig.empty()) {
            cfg.cameraBlendSig = kCameraBlendSig;
        }
        log.info("Config loaded mode={}", headbob::modeToString(cfg.mode));
    }
    return true;
}

bool RealisticHeadBobMod::enable() {
    auto& log = getSelf().getLogger();
    // No ModuleBuilder — Levi 1.5.25 crashed in registerModule.

    if (!resolveAndHook()) {
        log.warn("CameraBlendSystemTick hook failed — check cameraBlendSig / MC version");
    } else {
        log.info("CameraBlendSystemTick hooked");
    }
    log.info("enable done mode={}", headbob::modeToString(headbob::Config::get().mode));
    return true;
}

bool RealisticHeadBobMod::disable() {
    unhookAll();
    return true;
}

bool RealisticHeadBobMod::unload() {
    unhookAll();
    return true;
}

bool RealisticHeadBobMod::resolveAndHook() {
    auto& log = getSelf().getLogger();
    auto& cfg = headbob::Config::get();

    std::string sig = cfg.cameraBlendSig.empty() ? kCameraBlendSig : cfg.cameraBlendSig;
    const std::string modName = cfg.moduleName.empty() ? "libminecraftpe.so" : cfg.moduleName;

    const uintptr_t addr = pl::memory::resolveSignature(sig, modName);
    if (addr == 0) {
        log.error("resolveSignature failed for CameraBlendSystemTick");
        return false;
    }

    log.info("CameraBlendSystemTick @ {:#x}", static_cast<unsigned long long>(addr));

    mHookTarget = reinterpret_cast<void*>(addr);
    if (pl::memory::hook(mHookTarget,
                         reinterpret_cast<void*>(&RealisticHeadBobMod::cameraBlendDetour),
                         reinterpret_cast<void**>(&mOrigCameraBlend)) != 0) {
        log.error("pl::memory::hook failed");
        mHookTarget = nullptr;
        mOrigCameraBlend = nullptr;
        return false;
    }
    return true;
}

void RealisticHeadBobMod::unhookAll() {
    if (mHookTarget && mOrigCameraBlend) {
        pl::memory::unhook(mHookTarget,
                           reinterpret_cast<void*>(&RealisticHeadBobMod::cameraBlendDetour));
    }
    mHookTarget = nullptr;
    mOrigCameraBlend = nullptr;
}

void RealisticHeadBobMod::cameraBlendDetour(void* component, void* blendState, float factor) {
    auto& self = RealisticHeadBobMod::instance();
    if (self.mOrigCameraBlend) {
        self.mOrigCameraBlend(component, blendState, factor);
    }
    self.applyToCamera(component, factor);
}

void RealisticHeadBobMod::applyToCamera(void* component, float /*factor*/) {
    auto& cfg = headbob::Config::get();
    if (!cfg.enabled || !component) return;

    // dt from render path; clamp like CameraOverhaul
    float dt = 1.f / 60.f;
    if (dt <= 0.f || dt > 0.1f) dt = 1.f / 60.f;

    static float gameTime = 0.f;
    gameTime += dt;

    // Player velocity stubs — optional NormalTick later
    const float speedXZ   = 0.f;
    const bool  onGround  = true;
    const bool  sprinting = false;
    const bool  mounted   = false;
    const bool  firstPerson = true;

    const auto delta = headbob::update(
        mState, cfg, dt,
        speedXZ, onGround, sprinting, mounted, firstPerson, gameTime);

    // HeadBob returns degrees → radians for local bias
    float pitchBias = delta.pitch * kDeg2Rad;
    float yawBias   = delta.yaw * kDeg2Rad;
    float rollBias  = 0.f;

    if (!std::isfinite(pitchBias) || !std::isfinite(yawBias)) return;
    if (std::fabs(pitchBias) < 1e-7f && std::fabs(yawBias) < 1e-7f) return;

    pitchBias = clampf(pitchBias, -0.7f, 0.7f);
    yawBias   = clampf(yawBias, -0.7f, 0.7f);

    auto* quat = reinterpret_cast<Quat*>(
        reinterpret_cast<std::uintptr_t>(component) + kRotationOffset);

    const float len = std::sqrt(quat->x * quat->x + quat->y * quat->y +
                                quat->z * quat->z + quat->w * quat->w);
    if (!(len > 0.5f && len < 1.5f)) return;

    Quat d = axisAngle(1.f, 0.f, 0.f, pitchBias);
    if (yawBias != 0.f) d = quatMul(d, axisAngle(0.f, 1.f, 0.f, yawBias));
    if (rollBias != 0.f) d = quatMul(d, axisAngle(0.f, 0.f, 1.f, rollBias));

    *quat = quatNormalize(quatMul(*quat, d));
}

LL_REGISTER_MOD(RealisticHeadBobMod, RealisticHeadBobMod::instance());
