#include "mod/MyMod.h"

#include <pl/Mod.hpp>
#include <pl/ModMenu.hpp>
#include <pl/memory/Hook.hpp>
#include <pl/memory/Signature.hpp>

#include <filesystem>
#include <string>

// Same approach as CameraOverhaul:
//   resolveSignature(byte pattern, "libminecraftpe.so")
//   pl::memory::hook(target, detour, &original, priority)
//
// Detour shape from CameraOverhaul symbols:
//   _cameraBlendTick_hook(void*, void*, float)
//
// Without cameraBlendSig for your MC version, mod loads but camera does not move.

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
    // Optional next step: LocalPlayer velocity / onGround signatures
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

    const auto cfgPath = getSelf().getModDir() / "config" / "config.json";
    auto& cfg = headbob::Config::get();

    if (!cfg.loadFromFile(cfgPath.string())) {
        cfg.loadDefaults();

        std::error_code ec;
        std::filesystem::create_directories(cfgPath.parent_path(), ec);

        cfg.saveToFile(cfgPath.string());
        log.info("Wrote default config");
    } else {
        log.info("Config loaded mode={}", headbob::modeToString(cfg.mode));
    }

    return true;
}

bool RealisticHeadBobMod::enable() {
    auto& log = getSelf().getLogger();
    auto& cfg = headbob::Config::get();

    using namespace pl::modmenu;

    ModuleBuilder("realistic_headbob.main", "Realistic Head Bob")
        .modId(getSelf().getId())
        .description("Step head-bob. Modes: default / bodycam / comfort / custom")
        .defaultEnabled(cfg.enabled)
        .onToggle([this](std::string_view, bool on) {
            headbob::Config::get().enabled = on;

            const auto p =
                getSelf().getModDir() / "config" / "config.json";

            headbob::Config::get().saveToFile(p.string());
        })
        .config("mode", "Mode", ConfigType::Radio, "bodycam",
                "default", "bodycam", "comfort", "custom")
        .config("globalStrength", "Global Strength",
                ConfigType::SliderFloat, "0.32", "0.0", "3.0")
        .config("stepStrength", "Step Strength",
                ConfigType::SliderFloat, "1.0", "0.0", "3.0")
        .config("swayStrength", "Sway Strength",
                ConfigType::SliderFloat, "1.0", "0.0", "3.0")
        .config("smoothHz", "Spring Hz",
                ConfigType::SliderFloat, "1.75", "0.25", "6.0")
        .config("damping", "Damping",
                ConfigType::SliderFloat, "1.25", "0.1", "3.0")
        .registerModule();

    if (!resolveAndHook()) {
        log.warn(
            "Camera hook not installed — set cameraBlendSig in config.json"
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

    // Apply gLastDelta to camera fields when offsets are known
    // (same stage as CameraOverhaul).
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