#pragma once

#include <ll/api/mod/NativeMod.h>

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

private:
    ll::mod::NativeMod& mSelf;
    headbob::State      mState{};

    using CameraBlendFn = void (*)(void* component, void* blendState, float factor);
    CameraBlendFn mOrigCameraBlend = nullptr;
    void*         mHookTarget      = nullptr;

    bool resolveAndHook();
    void unhookAll();

    // Same signature as CameraOverhaul CameraBlendSystemTick
    static void cameraBlendDetour(void* component, void* blendState, float factor);
    void applyToCamera(void* component, float factor);
};
