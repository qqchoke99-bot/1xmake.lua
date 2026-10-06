#include "mod/HeadBob.h"
#include <algorithm>
#include <cmath>

namespace headbob {

namespace {

float clamp(float v, float lo, float hi) {
    return std::max(lo, std::min(hi, v));
}

float alphaFrom60Hz(float dt, float hz) {
    float a = 1.f - std::exp(-hz * dt * 60.f);
    return clamp(a, 0.f, 1.f);
}

float smootherstep(float t) {
    t = clamp(t, 0.f, 1.f);
    return t * t * t * (t * (t * 6.f - 15.f) + 10.f);
}

// Spring-damper identical to the original Java Realistic Bobbing mod
float spring(float& out, float& vel, float target,
             float dt, float hz, float damp)
{
    hz   = clamp(hz,   0.1f, 12.f);
    damp = clamp(damp, 0.05f, 3.f);

    const float omega = 6.2831855f * hz;
    const float k     = omega * omega;
    const float c     = 2.f * damp * omega;

    vel += (k * (target - out) - c * vel) * dt;
    out += vel * dt;
    return out;
}

} // namespace

CameraDelta update(State& s,
                   const Config& cfg,
                   float dt,
                   float horizontalSpeed,
                   bool  onGround,
                   bool  isSprinting,
                   bool  isMounted,
                   bool  isFirstPerson,
                   float gameTime)
{
    CameraDelta d{};

    if (!cfg.enabled) return d;
    if (isMounted && !cfg.enableWhenMounted) return d;
    if (!isFirstPerson && !cfg.enableThirdPerson) return d;

    // ---- preset multipliers ----
    float g  = cfg.globalStrength;
    float st = cfg.stepStrength;
    float sw = cfg.swayStrength;
    float co = cfg.cornerStrength;
    float br = cfg.breathStrength;
    float dr = cfg.driftStrength;

    switch (cfg.mode) {
        case Mode::Comfort:
            g *= 0.95f; st *= 0.90f; sw *= 0.85f; co *= 0.90f;
            br *= 1.10f; dr *= 1.10f;
            break;
        case Mode::Bodycam:
            g *= 1.10f; st *= 1.05f; sw *= 1.05f; co *= 1.10f;
            br *= 1.15f; dr *= 1.20f;
            break;
        case Mode::Default:
            // balanced — no extra multipliers
            break;
        case Mode::Custom:
        default:
            break;
    }

    g *= isSprinting ? cfg.runMult : cfg.walkMult;

    // ---- speed smoothing ----
    const float aSpeed = alphaFrom60Hz(dt, 0.14f);
    s.speedSmooth += (horizontalSpeed - s.speedSmooth) * aSpeed;

    if (!onGround) ++s.airTicks;
    else           s.airTicks = 0;

    const bool moving = (s.speedSmooth > 0.01f) && onGround && (s.airTicks <= 2);
    const float targetFade = moving ? 1.f : 0.f;

    // fadeLerp is stored as a small value; scale similarly to original
    const float aFade = alphaFrom60Hz(dt, cfg.fadeLerp * 60.f);
    s.bobFade += (targetFade - s.bobFade) * aFade;
    s.bobFade  = clamp(s.bobFade, 0.f, 1.f);

    if (s.bobFade < 1e-4f && !cfg.idleEnabled)
        return d;

    // ---- step phase (movement-synced) ----
    const float stepFreq = 1.7f * cfg.stepRate * (0.6f + s.speedSmooth * 0.8f);
    const float phase    = gameTime * stepFreq * 6.2831855f;

    const float stepSin = std::sin(phase);
    const float stepCos = std::cos(phase);

    // vertical dip
    const float stepPitch = stepSin * st * 1.6f * s.bobFade * g;
    // side sway
    const float stepYaw   = stepCos * sw * 1.25f * s.bobFade * g;
    // corner weight (diagonal feel without real roll)
    const float corner    = stepSin * stepCos * co * 0.55f * s.bobFade * g;

    float targetYaw   = stepYaw + corner;
    float targetPitch = stepPitch;

    // ---- idle breathing + handheld drift ----
    float idleFactor = 1.f - s.bobFade;
    idleFactor = smootherstep(clamp(idleFactor, 0.f, 1.f));

    if (cfg.idleEnabled && idleFactor > 0.01f) {
        const float idleAmp = idleFactor * cfg.idleShakeMult;

        const float breath = std::sin(gameTime * cfg.breathRate * 6.2831855f)
                           * br * 1.25f * idleAmp * g;

        const float driftY = std::sin(gameTime * cfg.driftRate * 6.2831855f + 1.2f)
                           * dr * 0.55f * idleAmp * g;

        const float driftP =
              std::sin(gameTime * cfg.driftRate * 6.2831855f * 0.62f + 0.9f)
            * dr * 0.60f * idleAmp * g
            + std::sin(gameTime * cfg.driftRate * 6.2831855f * 0.31f + 1.7f)
            * dr * 0.40f * idleAmp * g;

        targetYaw   += driftY;
        targetPitch += breath + driftP;
    }

    targetYaw   = clamp(targetYaw,   -8.f, 8.f);
    targetPitch = clamp(targetPitch, -8.f, 8.f);

    // ---- spring (the "realistic" inertia) ----
    spring(s.yawOut,   s.yawVel,   targetYaw,   dt, cfg.smoothHz, cfg.damping);
    spring(s.pitchOut, s.pitchVel, targetPitch, dt, cfg.smoothHz, cfg.damping);

    d.yaw   = s.yawOut;
    d.pitch = s.pitchOut;
    return d;
}

} // namespace headbob
