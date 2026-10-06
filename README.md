# Realistic Head Bob (LeviLauncher)

Step / bodycam head-bob for Bedrock on **Levi Launcher**.  
Uses the same PL stack as CameraOverhaul (`resolveSignature` + `hook`).

## Modes

| Mode | Feel |
|------|------|
| `default` | balanced |
| `bodycam` | heavier (default) |
| `comfort` | gentler |
| `custom` | use sliders only |

## Pre-filled signature (MC 1.26.32x / v126322)

From Zaphkiel — function xref of  
`"Multiplayer level - tick camera systems"` @ `0x9f13e0c`

```text
F4 4F 06 A9 FD 03 01 91 54 D0 3B D5 F3 03 00 AA 88 ?? ?? F9 A8 83 1F F8 ?? ?? ?? D0 08 A1 11 91 08 FD DF 08 ?? ?? ?? ?? ?? ?? ?? D0 21 E0 0C 91 E0 63 00 91 ?? ?? ?? 95
```

- `unique: true`
- `matches: 1`
- ARM64 prologue (`stp` / `mrs tpidr_el0`)

Written into default `config.json` on first run.

> This function is the **tick-camera-systems** path (profiler/system registration side).  
> If camera does not move after hook, the real per-frame blend may be a callee — dig callers of `0x9f13e0c` next.  
> Keep **CameraOverhaul** for cinematic pitch/roll/sway; this mod adds step bob on top (`HookPriority::Low`).

## Build on GitHub

1. Push this folder as a repo  
2. Actions → **Build Levi Mod**  
3. Download `realistic_headbob-arm64-v8a` artifact (`.levipack`)  
4. Import in LeviLauncher under your MC version folder  

## Config path (after install)

```text
.../mods/realistic_headbob/config/config.json
```

Example:

```json
{
  "enabled": true,
  "mode": "bodycam",
  "globalStrength": 0.32,
  "moduleName": "libminecraftpe.so",
  "cameraBlendSig": "F4 4F 06 A9 FD 03 01 91 54 D0 3B D5 F3 03 00 AA 88 ?? ?? F9 A8 83 1F F8 ?? ?? ?? D0 08 A1 11 91 08 FD DF 08 ?? ?? ?? ?? ?? ?? ?? D0 21 E0 0C 91 E0 63 00 91 ?? ?? ?? 95"
}
```

## Status

| Feature | Status |
|---------|--------|
| Mode select (bodycam/default/...) | yes |
| Spring-damper step bob (from Java Realistic Head Bobbing) | yes |
| PL hook + signature | yes (this build) |
| Player speed / onGround sync | stub (idle + fixed step still run) |
| Apply yaw/pitch into camera struct | needs offset once hook lands |

## License / credit

Logic ported from [Realistic Head Bobbing](https://www.curseforge.com/minecraft/mc-mods/realistic-head-bobbing) (Java).  
Hook pattern follows CameraOverhaul / preloader-android.
