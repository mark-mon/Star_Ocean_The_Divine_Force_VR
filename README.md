# Star Ocean The Divine Force VR

A native VR mod for **Star Ocean The Divine Force** (Steam, PC). You play in third person with a gamepad as
usual; the game is rendered in true per-eye stereo on your SteamVR headset through OpenXR.

## Features

- **Native stereo:** each eye is rendered separately, with real depth. This is not alternate-eye rendering (AER).
- **Head tracking:** look and lean around the game world.
- **Rendered at your headset's resolution:** the mod uses the headset's recommended per-eye resolution, with a render scale.
- **HUD on its own panel:** the HUD floats in front of you. You can set its distance, size, height, and whether it stays fixed in the world or follows your head.
- **In-headset settings menu:** open it with **Insert**, or hold **L3 + R3** on the gamepad. Changes apply live and are saved to `config.txt`.
- **Foveated rendering (NVIDIA RTX):** the outer area of each eye is shaded at a lower rate for more fps. Fixed mode keeps the sharp area at the centre of each lens. Eye-tracked mode moves it with your gaze on headsets with eye tracking (OpenXR `XR_EXT_eye_gaze_interaction`).
- **TAA control:** turn the game's TAA off, reduce its camera jitter, or add sharpening to counter TAA softness.
- **Smooth output:** frames are sent to the headset at its full refresh rate, independent of the game's frame rate. The mod corrects each frame for your latest head rotation.

## Requirements

- Windows 10/11 and the **Steam** version of the game. The mod checks the game build and does nothing on an unknown one.
- A SteamVR headset, with SteamVR set as the OpenXR runtime.
- A gamepad.
- **A strong PC.** Native stereo renders the scene twice per frame, so expect a high CPU and GPU load. Foveated rendering needs an NVIDIA RTX GPU.

## Install

1. Download the zip from [Releases](../../releases).
2. Copy `xinput9_1_0.dll` and `openxr_loader.dll` into the game folder, next to `SO6.exe`. For example: `...\steamapps\common\STAR OCEAN THE DIVINE FORCE\`.
3. Start SteamVR, then start the game from Steam.

On first start the mod creates `config.txt` and `so6vr.log` in the game folder.

**Uninstall:** delete `xinput9_1_0.dll` and `openxr_loader.dll` from the game folder. You can also delete `config.txt` and `so6vr.log`.

## Controls

| Key | Action |
|---|---|
| Insert, or hold L3 + R3 | Open or close the settings menu |
| F8 | Switch between the flat virtual screen and stereo |
| F9 | Recentre the view (also moves the HUD panel) |
| F10 | Show or hide the HUD panel |

The title screen is always shown on a flat virtual screen; stereo starts in game.

## Settings

Everything is in `config.txt` in the game folder. Each setting is documented there, and most can also be
changed in the settings menu. The most useful ones:

| Setting | Values | Notes |
|---|---|---|
| `RenderMode` | `Flat`, `Native` | Flat = virtual screen; Native = full stereo |
| `RenderResolution` | `Auto`, `Game`, `WxH` | Auto = the headset's recommended per-eye size |
| `RenderScale` | 0.5–2.0 | Lower for more fps, higher for sharpness |
| `Foveation` | `Off`, `Fixed`, `Gaze` | NVIDIA RTX only. Gaze needs eye tracking (falls back to Fixed) |
| `FoveationStrength` | `Low`, `Medium`, `High` | Start with Medium |
| `TAA` | `true`, `false` | false = sharpest image, but edges shimmer |
| `TaaJitter` | 0.0–1.0 | Lower = less TAA blur, more aliasing |
| `Sharpen` | 0.0–1.0 | Sharpening of the headset image |
| `HudDistance`, `HudWidth`, `HudOffsetY`, `HudAnchor` | metres; `World` / `Head` | HUD panel placement |
| `MenuDistance`, `MenuWidth`, `MenuOffsetY`, `MenuAnchor` | metres; `World` / `Head` | Settings menu placement |
| `WorldScale` | 1.0 = real scale | Larger values exaggerate head movement and depth |

## Troubleshooting

- **Nothing in the headset:** check that SteamVR is running and is the active OpenXR runtime. Then look in `so6vr.log`.
- **Low frame rate:** lower `RenderScale` (for example 0.8), or turn on `Foveation = Fixed`.
- **Image too soft:** try `TaaJitter = 0.5`, `Sharpen = 0.3`, or `TAA = false`.
- **Reporting a problem:** please attach `so6vr.log`, and for crashes the game's `SO6.crash.*` files.

## How it works (short version)

- **Loading:** the mod is an `xinput9_1_0.dll` proxy that the game loads at startup.
- **Stereo:** the game renders the left eye with a head-tracked camera. The mod then replays that frame's render commands for the right eye, with the camera matrices swapped at constant-upload time. Temporal effects, TAA's linear-depth state and the game's frame-history textures are kept separate per eye.
- **HUD:** UI draws are redirected to their own texture. A black/white matte recovers correct transparency.
- **Headset output:** a separate thread with its own D3D11 device runs the OpenXR frame loop at the headset's refresh rate.

Technical notes are in [`docs/`](docs).

## Building

The DLL is cross-compiled on Linux with clang-cl and lld-link.

**What you need:**
- LLVM 22 (the MSVC STL needs Clang ≥ 22).
- The MSVC STL/CRT and Windows SDK headers and libraries. Copy them from your own Visual Studio installation; they are not included here.
- [MinHook](https://github.com/TsudaKageyu/minhook).
- The [OpenXR SDK](https://github.com/KhronosGroup/OpenXR-SDK) headers and the prebuilt Windows loader.

**Steps:**
1. Run `scripts/setup_env.sh /path/to/toolchain.zip`. It sets everything up (paths in `scripts/env.sh`).
2. Run `scripts/build.sh`. The output is `build/xinput9_1_0.dll` and `build/openxr_loader.dll`.

## Credits

- Dear ImGui (Omar Cornut), MinHook (Tsuda Kageyu), the Khronos OpenXR SDK and loader, and the RenderDoc in-application API header.
- See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

This is an unofficial fan mod. It is not affiliated with or endorsed by Square Enix or tri-Ace. You need
your own legal copy of the game.
