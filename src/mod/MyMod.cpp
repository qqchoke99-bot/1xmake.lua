#include "mod/MyMod.h"

#include <pl/Mod.hpp>
#include <pl/memory/Hook.hpp>
#include <pl/memory/Signature.hpp>

#include <filesystem>
#include <string>

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

    log.info("Realistic Head Bob: load");

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

        if (!cfg.saveToFile(cfgPath.string())) {
            log.warn("Failed to save default config");
        } else {
            log.info("Wrote default config");
        }
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

    log.info("Realistic Head Bob: enable");

    /*
     * ModMenu is intentionally disabled.
     *
     * Previous crash:
     *   pl::modmenu::registerModule()
     *
     * So ModMenu is NOT involved in this test.
     */

    log.info("ModMenu disabled");

    /*
     * CameraBlend hook test.
     *
     * The hook will be installed, but the detour below
     * intentionally does absolutely nothing.
     *
     * This isolates the hook mechanism itself.
     */

    log.info("Installing CameraBlend test hook");

    if (!resolveAndHook()) {
        log.error("CameraBlend test hook FAILED");
    } else {
        log.info("CameraBlend test hook INSTALLED");
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
        log.error("cameraBlendSig is empty");
        return false;
    }

    log.info(
        "Resolving CameraBlend signature in {}",
        cfg.moduleName
    );

    const uintptr_t addr =
        pl::memory::resolveSignature(
            cfg.cameraBlendSig,
            cfg.moduleName
        );

    if (addr == 0) {
        log.error("CameraBlend signature NOT found");
        return false;
    }

    log.info(
        "CameraBlend signature resolved @ {:#x}",
        static_cast<unsigned long long>(addr)
    );

    /*
     * Install the hook.
     */
    mCameraBlendHook = pl::memory::HookHandle(
        reinterpret_cast<void*>(addr),
        reinterpret_cast<void*>(
            &RealisticHeadBobMod::cameraBlendDetour
        ),
        reinterpret_cast<void**>(
            &mOrigCameraBlend
        ),
        pl::memory::HookPriority::Low
    );

    if (!mCameraBlendHook.installed()) {
        log.error("CameraBlend HookHandle installation FAILED");

        mOrigCameraBlend = nullptr;

        return false;
    }

    log.info(
        "CameraBlend HookHandle installation SUCCESS"
    );

    /*
     * IMPORTANT:
     *
     * We intentionally do NOT call the original function
     * from the detour in this test.
     */

    return true;
}

void RealisticHeadBobMod::unhookAll() {
    mCameraBlendHook.reset();
    mOrigCameraBlend = nullptr;
}

/*
 * ============================================================
 * CameraBlend TEST DETOUR
 * ============================================================
 *
 * This function intentionally does NOTHING.
 *
 * No:
 *   - original CameraBlend call
 *   - HeadBob calculation
 *   - memory writes
 *   - camera modification
 *   - player lookup
 *
 * If the game crashes with this detour, the problem is very
 * likely related to the hook target/signature/hook mechanism.
 */

void RealisticHeadBobMod::cameraBlendDetour(
    void* a,
    void* b,
    float dt
) {
    (void)a;
    (void)b;
    (void)dt;

    /*
     * EMPTY INTENTIONALLY
     */
}

/*
 * ============================================================
 * HeadBob calculation
 * ============================================================
 *
 * Not used by the current crash-isolation test.
 */

void RealisticHeadBobMod::onCameraTick(float dt) {
    auto& cfg = headbob::Config::get();

    if (!cfg.enabled)
        return;

    if (dt <= 0.f || dt > 0.1f) {
        dt = 1.f / 60.f;
    }

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
