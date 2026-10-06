#pragma once
#include "mod/Config.h"

namespace headbob {

struct State {
    float bobFade     = 0.f;
    float speedSmooth = 0.f;
    float yawOut      = 0.f;
    float yawVel      = 0.f;
    float pitchOut    = 0.f;
    float pitchVel    = 0.f;
    int   airTicks    = 0;
};

struct CameraDelta {
    float yaw   = 0.f;  // degrees
    float pitch = 0.f;
};

/**
 * Update head-bob state and return camera angle offsets (degrees).
 *
 * @param s               persistent state (keep across frames)
 * @param cfg             current config
 * @param dt              delta time in seconds
 * @param horizontalSpeed player XZ speed (blocks/s)
 * @param onGround        true if player is on ground
 * @param isSprinting     true if sprinting
 * @param isMounted       true if riding boat/horse/minecart/...
 * @param isFirstPerson   true if first-person camera
 * @param gameTime        world/game time in seconds (for phase)
 */
CameraDelta update(State& s,
                   const Config& cfg,
                   float dt,
                   float horizontalSpeed,
                   bool  onGround,
                   bool  isSprinting,
                   bool  isMounted,
                   bool  isFirstPerson,
                   float gameTime);

} // namespace headbob
