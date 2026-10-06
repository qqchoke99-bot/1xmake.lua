#include "mod/MyMod.h"

#include <pl/Mod.hpp>
#include <pl/memory/Hook.hpp>
#include <pl/memory/Signature.hpp>

#include <filesystem>
#include <string>

// CameraBlend hook approach:
//   resolveSignature(byte pattern, "libminecraftpe.so")
//   HookHandle(target, detour, &original, priority)
//
// Detour shape:
//   cameraBlendDetour(void*, void*, float)
//
// IMPORTANT:
// ModMenu registration is intentionally disabled for this test.
// The previous crash occurred inside:
//   pl::modmenu::registerModule()
// We keep the actual mod lifecycle, config, signature resolver,
// hook, and HeadBob logic unchanged.

namespace {

struct CamWrite {
    float yaw   = 0.f;
    float pitch = 0.f;
};

CamWrite gLastDelta{};

struct PlayerSnap {
    float speedXZ     = 0.f;
    bool  onGround    = true;
    bool  sprinting   = false;
    bool  mounted     = false;
    bool  firstPerson = true;
};

PlayerSnap queryPlayerRough() {
    // Optional next step:
    // LocalPlayer velocity / onGround signatures.
    return {};
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

    const auto cfgPath =
        getSelf().getModDir() / "config" / "config.json";

    auto& cfg = headbob::Config::get();

    if (!cfg.loadFromFile(cfgPath.string())) {
        cfg.loadDefaults();

        std::error_code ec;

        std::filesystem::create_directories(
            cfgPath.parent_path(),
            ec
        );

        cfg.saveToFile(cfgPath.string());

        log.info("Wrote default config");
    } else {
        log.info(
            "Config loaded mode={}",
            headbob::modeToString(cfg.mode)
        );
    }

    return true;
}

bool RealisticHeadBobMod::enable() {
    auto& log = getSelf().getLogger();

    auto& cfg = headbob::Config::get();

    log.info("Realistic Head Bob enable");

    /*
     * ModMenu registration is temporarily disabled.
     *
     * Previous crash:
     *
     *   #01 libpreloader.so
     *       pl::modmenu::registerModule(...)
     *
     *   #02 librealistic_headbob.so
     *       RealisticHeadBobMod::enable()
     *
     * Therefore we do NOT call ModuleBuilder/registerModule()
     * until the runtime compatibility issue is isolated.
     *
     * Configuration still comes from:
     *
     *   config/config.json
     */

    if (!cfg.enabled) {
        log.info("Realistic Head Bob disabled by config");
        return true;
    }

    log.info("ModMenu registration skipped");

    if (!resolveAndHook()) {
        log.warn(
            "Camera hook not installed — "
            "check cameraBlendSig in config.json"
        );
    } else {
        log.info("Camera blend hook OK");
    }

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

    if (cfg.cameraBlendSig.empty()) {
        log.warn("cameraBlendSig empty");
        return false;
    }

    const uintptr_t addr =
        pl::memory::resolveSignature(
            cfg.cameraBlendSig,
            cfg.moduleName
        );

    if (addr == 0) {
        log.error("resolveSignature failed");
        return false;
    }

    log.info(
        "cameraBlend @ {:#x}",
        static_cast<unsigned long long>(addr)
    );

    mCameraBlendHook = pl::memory::HookHandle(
        reinterpret_cast<void*>(addr),
        reinterpret_cast<void*>(
            &RealisticHeadBobMod::cameraBlendDetour
        ),
        reinterpret_cast<void**>(&mOrigCameraBlend),
        pl::memory::HookPriority::Low
    );

    return mCameraBlendHook.installed();
}

void RealisticHeadBobMod::unhookAll() {
    mCameraBlendHook.reset();
    mOrigCameraBlend = nullptr;
}

void RealisticHeadBobMod::cameraBlendDetour(
    void* a,
    void* b,
    float dt
) {
    auto& self = RealisticHeadBobMod::instance();

    if (self.mOrigCameraBlend) {
        self.mOrigCameraBlend(a, b, dt);
    }

    self.onCameraTick(dt);

    // Apply gLastDelta to camera fields when offsets are known.
    // This remains the same stage as CameraOverhaul.

    (void)b;
    (void)gLastDelta;
}

void RealisticHeadBobMod::onCameraTick(float dt) {
    auto& cfg = headbob::Config::get();

    if (!cfg.enabled)
        return;

    if (dt <= 0.f || dt > 0.1f)
        dt = 1.f / 60.f;

    static float gameTime = 0.f;

    gameTime += dt;

    const auto snap = queryPlayerRough();

    const auto delta = headbob::update(
        mState,
        cfg,
        dt,
        snap.speedXZ,
        snap.onGround,
        snap.sprinting,
        snap.mounted,
        snap.firstPerson,
        gameTime
    );

    gLastDelta.yaw   = delta.yaw;
    gLastDelta.pitch = delta.pitch;
}

PL_REGISTER_MOD(
    RealisticHeadBobMod,
    RealisticHeadBobMod::instance()
);