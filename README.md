# Realistic Head Bob (LeviLauncher)

FPS-style step head-bob for Bedrock.  
Camera path matches **CameraOverhaul 1.1.2-beta**.

## How camera moves

Same as CameraOverhaul:

1. Resolve `CameraBlendSystemTick` in `libminecraftpe.so`
2. Hook `void(component, blendState, factor)`
3. After original, multiply quaternion at `component + 0x28`

Signature (from CO `Signatures.cpp`):

```text
? ? ? D1 ? ? ? 6D ? ? ? 6D ? ? ? 6D ? ? ? 6D ? ? ? A9 ? ? ? F9 ? ? ? A9 ? ? ? A9 ? ? ? 91 55 D0 3B D5 F3 03 01 AA F4 03 00 AA ? ? ? F9 ? ? ? 91 ? ? ? 91
```

Official CO targets **1.26.45.1**. Nearby 1.26.x builds may work; if resolve fails, update `cameraBlendSig` in config.

## Modes

| Mode | Feel |
|------|------|
| `default` | balanced |
| `bodycam` | heavier (default) |
| `comfort` | gentler |
| `custom` | sliders only |

Edit: `mods/.../config/config.json`  
(No in-game ModuleBuilder — avoided Levi 1.5.25 crash.)

## Build

Push → Actions → **Build Levi Mod** → download `.levipack`

## Credits

- Head-bob math: Realistic Head Bobbing (Java)
- Camera hook / quat write: CameraOverhaul Bedrock (GPL-3.0, Mirsario-derived math)
