#pragma once
#include "vrkit_menu_placement.h"
#include <string>

namespace so6vr {

// Settings loaded from config.txt in the game folder. Unknown keys are ignored;
// missing keys keep these defaults. A default config.txt is written if none exists.
struct Config {
    bool enabled = true;          // master switch: false = pure XInput passthrough, no hooks
    bool verboseLog = false;      // extra per-event logging
    int  statsIntervalSec = 10;   // how often to log frame statistics (0 = off)

    // Stage 2: virtual screen in the headset
    bool  vrOutput = true;        // send the game image to the headset via OpenXR
    float screenDistance = 2.5f;  // metres in front of the seated position
    float screenWidth = 3.2f;     // metres; height follows the game's aspect ratio
    float screenHeight = 0.0f;    // metres above (+) or below (-) eye level at session start
    // Stage 3a: camera probe
    int   cameraProbeIntervalSec = 2; // seconds between camera log lines (0 = off)
    float debugLensShiftX = 0.0f;     // diagnostic: shift the picture (fraction of half width); 0 = off
    float debugLensShiftY = 0.0f;

    // Stage 3b: head tracking
    // Render modes. Flat = virtual screen; Mono = head-tracked single view (both eyes same image);
    // AER = alternate-eye rendering; Native = true per-eye stereo. AER/Native not implemented yet.
    enum Mode { Flat = 0, Mono = 1, AER = 2, Native = 3 };
    int   renderMode = Native;    // as configured in config.txt
    int   activeMode = Native;    // what is running now (F8 toggles between Flat and renderMode)
    enum ResMode { ResAuto = 0, ResGame = 1, ResExplicit = 2 };
    int   renderResMode = ResAuto;
    unsigned renderResW = 0, renderResH = 0;
    float renderScale = 1.0f;
    bool  taa = true;             // the game's temporal anti-aliasing (its own engine switch)
    float taaJitter = 1.0f;       // TAA camera jitter amount, 0..1 (lower = less shimmer-blur, more aliasing)
    float sharpen = 0.0f;
    int   foveation = 0;            // 0 = off, 1 = fixed, 2 = follows the eye-tracking gaze (NVIDIA VRS)
    bool  debugFakeGaze = false;    // diagnostic: circling fake gaze for tests without an eye tracker
    int   foveationStrength = 1;    // 0 Low, 1 Medium, 2 High
    float foveationInner = 0.45f;   // full-rate radius (1.0 = half the image width)
    float foveationOuter = 0.80f;   // beyond this radius: lowest rate         // sharpening of the headset image, 0..1 (counters TAA softness)
    bool  hudLayer = true;        // head-tracked modes: HUD on its own fixed layer instead of in the eye images
    bool  hudOnDesktop = true;    // draw the HUD back over the game image on the monitor
    float hudDistance = 1.5f;     // metres
    float hudWidth = 1.5f;        // metres (height follows the HUD's aspect)
    float hudOffsetY = -0.05f;    // metres, up from eye level
    int   hudAnchor = 0;
    bool  reticleInScene = true;  // draw the aim crosshair in the 3D image instead of on the HUD panel
    float reticleDistance = 10.0f; // metres along the aim direction
    vrkit::menu::Placement menu;  // settings menu panel: distance, width, height offset, anchor (World/Head)
    bool  debugMenuDump = false;
    bool  mouseFix = true;
    bool  queryWait = false;
    bool  patchFullResOnly = true;
    bool  replayGpuSync = false;
    bool  hudAlphaFix = false;    // old method: change the HUD's alpha blending (breaks the minimap); the matte is used instead // stereo camera shift only in passes near eye resolution      // diagnostic: wait for GPU query results instead of returning 'not ready'        // 1:1 cursor mapping at VR render sizes (stops the camera drifting)          // 0 = World (fixed in front of you, recentred with F9), 1 = Head (follows your head)
    bool  restoreCameraAfterPrepare = false; // 1 = put the game camera back right after frame preparation
    float worldScale = 1.0f;      // 1 = real scale; >1 makes head movement move the camera further
    bool  horizonLock = true;     // remove the game camera's pitch/roll so the horizon stays level
    float predictionFrames = 2.0f;// how far ahead (in headset frames) to predict the head pose
    float lensShiftYSign = 1.0f;
    int   frameMatchOffset = 0;
    int   reprojection = 1;           // 0 = runtime (SteamVR) reprojects; 1 = the mod warps each frame itself
    float debugLayerYawDeg = 0.0f;
    float debugFakeHeadYawDeg = 0.0f; // diagnostic: fake head turning (amplitude in degrees) for null-headset tests
    float debugFakeHeadHz = 0.25f;
    float debugRenderViewYawDeg = 0.0f; // diagnostic: rotate the render-side view by this many degrees
    int   debugReplay = 0;              // experiment: execute each frame a second time (1 = whole command range, 2 = render jobs only)
    float debugReplayYawDeg = 0.0f;
    bool  debugReplayViewSetup = true;
    bool  replayRestoreState = true;
    int   debugDesktopEye = 1;           // native stereo: which eye the monitor shows (0 = left, 1 = right)
    bool  stereoHistoryFix = true;       // per-eye copies of the scene/TAA history read across frames
    bool  stereoHistoryAll = true;       // ... and of every other history texture (found automatically)
    bool  stereoHistoryUav = true;       // also detect history updated in place through UAVs
    int   stereoHistoryMax = 16;
    int   debugHistLog = 0;
    bool  debugDumpKey = false;
    bool  debugFakePad = false;          // diagnostic: keyboard-driven pad 0 (F5 = RB)          // diagnostic: F11 writes both eye images              // diagnostic: list the textures pass 1 uses after this many tracked frames         // safety limit on how many textures get per-eye copies
    bool  debugLatTrack = false;
    int   replayRestoreMask = 14;    // 1 stateful passes (breaks TAA history), 2 globals, 4 small UAVs, 8 TAA depth state
    int   debugDumpEyesAt = 0;           // native stereo: write both eye images as BMP at this gameplay frame (0 = off)
    bool  replayReuploadBuffers = true;  // re-issue the frame's structured buffer uploads before the second-eye pass
    unsigned replayUavMaxBytes = 256;    // compute buffers up to this size are restored before the second-eye pass
    unsigned replayUavMaxTexels = 64;   // compute textures up to this many texels likewise     // restore stateful post-process objects before the second-eye pass   // experiment: rebuild the view constants before the replay     // experiment: rotate the view for the second execution
    std::wstring renderDocPath;         // diagnostic: folder with renderdoc.dll (empty = off)
    int   renderDocCaptureFrame = 600;  // capture after this many gameplay frames
    int   renderDocCaptureCount = 1;
    bool  debugStacks = false;        // diagnostic: log call stacks of key render events (a few times each)    // diagnostic: fake head turning frequency    // diagnostic: rotate the pose reported to SteamVR by this many degrees   // pair each image with the pose of frame (matched + offset); diagnostic  // flip (-1) if the picture is vertically off-centre in the headset

    bool  d3dMultithreadProtect = false; // D3D11 multithread protection; off: it reroutes the game's calls past our hooks
};

extern Config g_config;

// True when the current mode drives the game camera from the headset.
inline bool HeadTrackedMode() { return g_config.activeMode != Config::Flat; }
const char* ModeName(int mode);

void LoadConfig(const std::wstring& gameDir);

} // namespace so6vr
