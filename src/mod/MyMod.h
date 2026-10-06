#pragma once

#include <pl/Mod.hpp>
#include <pl/memory/Hook.hpp>

#include "mod/Config.h"
#include "mod/HeadBob.h"

class RealisticHeadBobMod {
public:
    static RealisticHeadBobMod& instance();

    RealisticHeadBobMod();

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    bool load();
    bool enable();
    bool disable();
    bool unload();

    void onCameraTick(float dt);

private:
    ll::mod::NativeMod& mSelf;
    headbob::State      mState{};

    using CameraBlendFn = void (*)(void*, void*, float);
    CameraBlendFn          mOrigCameraBlend = nullptr;
    pl::memory::HookHandle mCameraBlendHook{};

    bool resolveAndHook();
    void unhookAll();
    static void cameraBlendDetour(void* a, void* b, float dt);
};