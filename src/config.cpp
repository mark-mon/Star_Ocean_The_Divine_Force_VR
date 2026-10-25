#include <windows.h>
#include "config.h"
#include "log.h"
#include <cstdio>
#include <cstring>
#include <cctype>

namespace so6vr {

Config g_config;

static const char* kDefaultConfig =
    "# Star Ocean The Divine Force VR mod - configuration\n"
    "# Lines are key = value. Lines starting with # are comments.\n"
    "\n"
    "# Master switch. 0 = mod does nothing (controller passthrough only).\n"
    "Enabled = 1\n"
    "\n"
    "# Extra detail in so6vr.log (1 = on).\n"
    "VerboseLog = 0\n"
    "\n"
    "# Seconds between frame-statistics lines in so6vr.log (0 = off).\n"
    "StatsIntervalSec = 10\n"
    "\n"
    "# --- Headset output ---\n"
    "# 1 = show the game in the headset through OpenXR (SteamVR).\n"
    "VROutput = 1\n"
    "\n"
    "# Virtual screen placement, in metres, relative to your seated position when the game\n"
    "# connected to the headset. Use SteamVR's 'Reset seated position' to recentre.\n"
    "ScreenDistance = 2.5\n"
    "ScreenWidth = 3.2\n"
    "ScreenHeight = 0.0\n"
    "\n"
    "# How the game is shown in the headset:\n"
    "#   Flat   = a flat virtual screen in front of you\n"
    "#   Native = full per-eye stereo (each eye rendered separately)\n"
    "# The title screen always uses the flat screen.\n"
    "# Hotkeys in game: F8 switches between Flat and this mode, F9 recentres the view.\n"
    "RenderMode = Native\n"
    "\n"
    "# Resolution the game renders each eye at (Native):\n"
    "#   Auto = the headset's recommended per-eye resolution (from SteamVR)\n"
    "#   Game = the resolution chosen in the game's graphics options\n"
    "#   WxH  = a fixed size, e.g. 3840x3552\n"
    "RenderResolution = Auto\n"
    "# Multiplies the resolution above (0.5 - 2.0). Lower for more fps, higher for sharpness.\n"
    "RenderScale = 1.0\n"
    "\n"
    "# --- Image ---\n"
    "# The game's temporal anti-aliasing. false = sharpest image, but edges shimmer.\n"
    "TAA = true\n"
    "# TAA camera jitter (0.0 - 1.0). Lower = less TAA blur and smearing, more visible aliasing.\n"
    "TaaJitter = 1.0\n"
    "# Sharpening of the headset image (0.0 = off, 1.0 = strong). Counters TAA softness.\n"
    "Sharpen = 0.0\n"
    "\n"
    "# --- Foveated rendering (NVIDIA RTX GPUs) ---\n"
    "# Fixed = shade the outer part of each eye's image at a reduced rate for more fps (centred on each lens).\n"
    "# Gaze  = the full-quality area follows your eyes (headsets with eye tracking; falls back to Fixed).\n"
    "# Off   = full rate everywhere.\n"
    "Foveation = Off\n"
    "# How coarse the outer areas are: Low, Medium or High.\n"
    "FoveationStrength = Medium\n"
    "# Radius of the full-quality centre and where the lowest rate starts (1.0 = half the image width).\n"
    "FoveationInner = 0.45\n"
    "FoveationOuter = 0.80\n"
    "\n"
    "# --- HUD (Native) ---\n"
    "# 1 = show the game's HUD on its own flat panel in front of you, 0 = leave it in the 3D image.\n"
    "HudLayer = 1\n"
    "# Panel distance and width in metres, and height offset from eye level (negative = lower).\n"
    "HudDistance = 1.5\n"
    "HudWidth = 1.5\n"
    "HudOffsetY = -0.05\n"
    "# World = fixed in front of you (F9 recentres it), Head = follows your head.\n"
    "HudAnchor = World\n"
    "# 1 = also draw the HUD on the monitor image. F10 hides/shows the HUD panel.\n"
    "HudOnDesktop = 1\n"
    "# 1 = draw the aim crosshair (D.U.M.A. aim) in the 3D view at ReticleDistance metres instead of on the\n"
    "# HUD panel, so it is not seen double.\n"
    "ReticleInScene = 1\n"
    "ReticleDistance = 10\n"
    "\n"
    "# --- Settings menu (Insert, or hold L3+R3) ---\n"
    "# Panel distance and width in metres, and height offset from eye level.\n"
    "MenuDistance = 1.0\n"
    "MenuWidth = 0.75\n"
    "MenuOffsetY = -0.05\n"
    "# World = fixed in space where it opened (turn your head to read it), Head = follows your head.\n"
    "MenuAnchor = World\n"
    "\n"
    "# 1 = real-world scale for head movement. Larger values exaggerate head movement.\n"
    "WorldScale = 1.0\n"
    "\n"
    "# 1 = keep the horizon level (the game camera's up/down tilt is removed; look up and down with\n"
    "# your head). 0 = keep the game's camera tilt.\n"
    "HorizonLock = 1\n"
    "\n"
    "# Who corrects each frame for head movement between rendering and display:\n"
    "#   Mod     = the mod warps every frame to your current head direction (recommended)\n"
    "#   Runtime = leave it to SteamVR / the headset driver\n"
    "Reprojection = Mod\n"
    "\n"
    "# Head pose prediction, in headset frames. Lower if the view lags, raise if it overshoots.\n"
    "PredictionFrames = 2.0\n"
    "\n"
    "# --- Advanced / diagnostics ---\n"
    "# Seconds between camera readouts in so6vr.log (0 = off).\n"
    "CameraProbeIntervalSec = 2\n"
    "\n"
    "# Diagnostic picture shift (0 = off). E.g. DebugLensShiftX = 0.3 moves the image sideways;\n"
    "# used once to confirm the direction convention.\n"
    "DebugLensShiftX = 0\n"
    "DebugLensShiftY = 0\n"
    "\n"
    "# Diagnostic: rotate the head pose reported to SteamVR by this many degrees (0 = off).\n"
    "DebugLayerYawDeg = 0\n"
    "\n"
    "# Diagnostic: pair each game image with the head pose of a neighbouring frame (-1, 0, 1).\n"
    "# Leave at 0 unless asked to test.\n"
    "FrameMatchOffset = 0\n"
    "\n"
    "# Set to -1 if the head-tracked picture sits too high or low (vertical off-centre fix).\n"
    "LensShiftYSign = 1\n"
    "\n"
    "# 1 = put the game's own camera back right after each frame is prepared (older behaviour;\n"
    "# can cause parts of the picture to follow your head). Leave at 0.\n"
    "RestoreCameraAfterPrepare = 0\n"
    "\n"
    "# D3D11 multithread protection (1 = on). Leave at 0: turning it on hides the game's\n"
    "# rendering from the mod's hooks. Only for troubleshooting SteamVR problems.\n"
    "D3DMultithreadProtect = 0\n";

static std::string Trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && isspace((unsigned char)s[a])) ++a;
    while (b > a && isspace((unsigned char)s[b - 1])) --b;
    return s.substr(a, b - a);
}

static bool EqNoCase(const std::string& a, const char* b) {
    return _stricmp(a.c_str(), b) == 0;
}

const char* ModeName(int mode) {
    switch (mode) {
    case Config::Flat: return "Flat";
    case Config::Mono: return "Mono";
    case Config::AER: return "AER";
    case Config::Native: return "Native";
    default: return "?";
    }
}

void LoadConfig(const std::wstring& gameDir) {
    std::wstring path = gameDir + L"\\config.txt";
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"r") != 0 || !f) {
        LOGI("config.txt not found, writing defaults");
        FILE* w = nullptr;
        if (_wfopen_s(&w, path.c_str(), L"w") == 0 && w) { fputs(kDefaultConfig, w); fclose(w); }
        return;
    }
    char buf[512];
    int lineNo = 0;
    while (fgets(buf, sizeof(buf), f)) {
        ++lineNo;
        std::string line = Trim(buf);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) { LOGW("config.txt line %d ignored (no '=')", lineNo); continue; }
        std::string key = Trim(line.substr(0, eq));
        std::string val = Trim(line.substr(eq + 1));
        size_t hash = val.find('#');
        if (hash != std::string::npos) val = Trim(val.substr(0, hash));
        int iv = atoi(val.c_str());
        // booleans: 1/0, true/false, yes/no, on/off (the in-game menu writes true/false)
        bool bv = iv != 0 || EqNoCase(val, "true") || EqNoCase(val, "yes") || EqNoCase(val, "on");
        float fv = (float)atof(val.c_str());
        if      (EqNoCase(key, "Enabled"))          g_config.enabled = bv;
        else if (EqNoCase(key, "VerboseLog"))       g_config.verboseLog = bv;
        else if (EqNoCase(key, "StatsIntervalSec")) g_config.statsIntervalSec = iv < 0 ? 0 : iv;
        else if (EqNoCase(key, "VROutput"))         g_config.vrOutput = bv;
        else if (EqNoCase(key, "ScreenDistance"))   g_config.screenDistance = fv < 0.3f ? 0.3f : fv;
        else if (EqNoCase(key, "ScreenWidth"))      g_config.screenWidth = fv < 0.2f ? 0.2f : fv;
        else if (EqNoCase(key, "ScreenHeight"))     g_config.screenHeight = fv;
        else if (EqNoCase(key, "CameraProbeIntervalSec")) g_config.cameraProbeIntervalSec = iv < 0 ? 0 : iv;
        else if (EqNoCase(key, "DebugLensShiftX"))  g_config.debugLensShiftX = fv;
        else if (EqNoCase(key, "DebugLensShiftY"))  g_config.debugLensShiftY = fv;
        else if (EqNoCase(key, "RenderResolution")) {
            unsigned w = 0, h = 0;
            if (EqNoCase(val, "Auto")) g_config.renderResMode = Config::ResAuto;
            else if (EqNoCase(val, "Game")) g_config.renderResMode = Config::ResGame;
            else if (sscanf(val.c_str(), "%ux%u", &w, &h) == 2 && w >= 320 && h >= 240) { g_config.renderResMode = Config::ResExplicit; g_config.renderResW = w; g_config.renderResH = h; }
            else LOGW("config: RenderResolution '%s' not understood (Auto, Game or WxH)", val.c_str());
        }
        else if (EqNoCase(key, "HudLayer")) g_config.hudLayer = bv;
        else if (EqNoCase(key, "HudOnDesktop")) g_config.hudOnDesktop = bv;
        else if (EqNoCase(key, "ReticleInScene")) g_config.reticleInScene = bv;
        else if (EqNoCase(key, "ReticleDistance")) g_config.reticleDistance = fv < 0.5f ? 0.5f : (fv > 200.0f ? 200.0f : fv);
        else if (EqNoCase(key, "HudDistance")) g_config.hudDistance = fv < 0.2f ? 0.2f : (fv > 20.0f ? 20.0f : fv);
        else if (EqNoCase(key, "HudWidth")) g_config.hudWidth = fv < 0.1f ? 0.1f : (fv > 20.0f ? 20.0f : fv);
        else if (EqNoCase(key, "HudOffsetY")) g_config.hudOffsetY = fv;
        else if (EqNoCase(key, "MenuDistance")) g_config.menu.distance = fv < 0.3f ? 0.3f : (fv > 5.0f ? 5.0f : fv);
        else if (EqNoCase(key, "HudAlphaFix")) g_config.hudAlphaFix = bv;
        else if (EqNoCase(key, "ReplayGpuSync")) g_config.replayGpuSync = bv;
        else if (EqNoCase(key, "PatchFullResOnly")) g_config.patchFullResOnly = bv;
        else if (EqNoCase(key, "QueryWait")) g_config.queryWait = bv;
        else if (EqNoCase(key, "MouseFix")) g_config.mouseFix = bv;
        else if (EqNoCase(key, "DebugMenuDump")) g_config.debugMenuDump = bv;
        else if (EqNoCase(key, "MenuOffsetY")) g_config.menu.offsetY = fv < -2.0f ? -2.0f : (fv > 2.0f ? 2.0f : fv);
        else if (EqNoCase(key, "MenuAnchor")) g_config.menu.anchor = EqNoCase(val, "Head") ? 1 : 0;
        else if (EqNoCase(key, "MenuWidth")) g_config.menu.width = fv < 0.2f ? 0.2f : (fv > 4.0f ? 4.0f : fv);
        else if (EqNoCase(key, "HudAnchor")) g_config.hudAnchor = EqNoCase(val, "Head") ? 1 : 0;
        else if (EqNoCase(key, "TAA")) g_config.taa = bv;
        else if (EqNoCase(key, "StereoHistoryAll")) g_config.stereoHistoryAll = bv;
        else if (EqNoCase(key, "StereoHistoryUav")) g_config.stereoHistoryUav = bv;
        else if (EqNoCase(key, "DebugHistLog")) g_config.debugHistLog = iv;
        else if (EqNoCase(key, "DebugDumpKey")) g_config.debugDumpKey = bv;
        else if (EqNoCase(key, "DebugFakePad")) g_config.debugFakePad = bv;
        else if (EqNoCase(key, "DebugPimaxEyes")) g_config.debugPimaxEyes = bv;
        else if (EqNoCase(key, "PatchScreenUV")) g_config.patchScreenUV = bv;
        else if (EqNoCase(key, "StereoHistoryMax")) g_config.stereoHistoryMax = iv < 0 ? 0 : (iv > 64 ? 64 : iv);
        else if (EqNoCase(key, "Foveation")) g_config.foveation = (EqNoCase(val, "Gaze") || EqNoCase(val, "Eye") || iv == 2) ? 2 : (EqNoCase(val, "Fixed") || EqNoCase(val, "true") || EqNoCase(val, "on") || iv == 1) ? 1 : 0;
        else if (EqNoCase(key, "DebugFakeGaze")) g_config.debugFakeGaze = bv;
        else if (EqNoCase(key, "FoveationStrength")) g_config.foveationStrength = EqNoCase(val, "Low") ? 0 : EqNoCase(val, "High") ? 2 : 1;
        else if (EqNoCase(key, "FoveationInner")) g_config.foveationInner = fv < 0.05f ? 0.05f : (fv > 2.0f ? 2.0f : fv);
        else if (EqNoCase(key, "FoveationOuter")) g_config.foveationOuter = fv < 0.05f ? 0.05f : (fv > 3.0f ? 3.0f : fv);
        else if (EqNoCase(key, "TaaJitter")) g_config.taaJitter = fv < 0.0f ? 0.0f : (fv > 1.0f ? 1.0f : fv);
        else if (EqNoCase(key, "Sharpen")) g_config.sharpen = fv < 0.0f ? 0.0f : (fv > 1.0f ? 1.0f : fv);
        else if (EqNoCase(key, "RenderScale")) g_config.renderScale = fv < 0.25f ? 0.25f : (fv > 3.0f ? 3.0f : fv);
        else if (EqNoCase(key, "RenderMode")) {
            if      (EqNoCase(val, "Flat"))   g_config.renderMode = Config::Flat;
            else if (EqNoCase(val, "Mono"))   g_config.renderMode = Config::Mono;
            else if (EqNoCase(val, "AER"))    g_config.renderMode = Config::AER;
            else if (EqNoCase(val, "Native")) g_config.renderMode = Config::Native;
            else if (iv >= 0 && iv <= 3 && !val.empty() && isdigit((unsigned char)val[0])) g_config.renderMode = iv;
            else LOGW("config.txt line %d: RenderMode '%s' not recognised (use Flat or Native)", lineNo, val.c_str());
        }
        else if (EqNoCase(key, "RestoreCameraAfterPrepare")) g_config.restoreCameraAfterPrepare = bv;
        else if (EqNoCase(key, "WorldScale"))       g_config.worldScale = fv < 0.05f ? 0.05f : fv;
        else if (EqNoCase(key, "HorizonLock"))      g_config.horizonLock = bv;
        else if (EqNoCase(key, "PredictionFrames")) g_config.predictionFrames = fv < 0 ? 0 : fv;
        else if (EqNoCase(key, "FrameMatchOffset")) g_config.frameMatchOffset = iv < -2 ? -2 : (iv > 2 ? 2 : iv);
        else if (EqNoCase(key, "Reprojection")) {
            if (EqNoCase(val, "Runtime") || val == "0") g_config.reprojection = 0;
            else if (EqNoCase(val, "Mod") || val == "1") g_config.reprojection = 1;
            else LOGW("config.txt line %d: Reprojection '%s' not recognised (use Runtime or Mod)", lineNo, val.c_str());
        }
        else if (EqNoCase(key, "DebugLayerYawDeg")) g_config.debugLayerYawDeg = fv;
        else if (EqNoCase(key, "DebugFakeHeadYawDeg")) g_config.debugFakeHeadYawDeg = fv;
        else if (EqNoCase(key, "DebugFakeHeadHz"))  g_config.debugFakeHeadHz = fv;
        else if (EqNoCase(key, "DebugRenderViewYawDeg")) g_config.debugRenderViewYawDeg = fv;
        else if (EqNoCase(key, "DebugReplay"))      g_config.debugReplay = iv;
        else if (EqNoCase(key, "ReplayUavMaxBytes")) g_config.replayUavMaxBytes = unsigned(iv < 0 ? 0 : iv);
        else if (EqNoCase(key, "ReplayUavMaxTexels")) g_config.replayUavMaxTexels = unsigned(iv < 0 ? 0 : iv);
        else if (EqNoCase(key, "ReplayReuploadBuffers")) g_config.replayReuploadBuffers = bv;
        else if (EqNoCase(key, "DebugDumpEyesAt")) g_config.debugDumpEyesAt = iv;
        else if (EqNoCase(key, "DebugDesktopEye")) g_config.debugDesktopEye = iv;
        else if (EqNoCase(key, "StereoHistoryFix")) g_config.stereoHistoryFix = bv;
        else if (EqNoCase(key, "DebugLatTrack")) g_config.debugLatTrack = bv;
        else if (EqNoCase(key, "ReplayRestoreMask")) g_config.replayRestoreMask = iv;
        else if (EqNoCase(key, "ReplayRestoreState")) g_config.replayRestoreState = bv;
        else if (EqNoCase(key, "DebugReplayViewSetup")) g_config.debugReplayViewSetup = bv;
        else if (EqNoCase(key, "DebugReplayYawDeg")) g_config.debugReplayYawDeg = fv;
        else if (EqNoCase(key, "RenderDocPath")) {
            wchar_t w[MAX_PATH]; MultiByteToWideChar(CP_UTF8, 0, val.c_str(), -1, w, MAX_PATH); g_config.renderDocPath = w;
        }
        else if (EqNoCase(key, "RenderDocCaptureFrame")) g_config.renderDocCaptureFrame = iv;
        else if (EqNoCase(key, "RenderDocCaptureCount")) g_config.renderDocCaptureCount = iv;
        else if (EqNoCase(key, "DebugStacks"))      g_config.debugStacks = bv;
        else if (EqNoCase(key, "LensShiftYSign"))   g_config.lensShiftYSign = fv < 0 ? -1.0f : 1.0f;
        else if (EqNoCase(key, "D3DMultithreadProtect")) g_config.d3dMultithreadProtect = bv;
        else LOGW("config.txt line %d: unknown key '%s'", lineNo, key.c_str());
    }
    fclose(f);
    // Mono still works when set by hand in config.txt (undocumented: not release quality, hidden from the menu).
    if (g_config.renderMode == Config::AER)
        LOGW("RenderMode AER is not implemented yet; running as Mono");
    g_config.activeMode = g_config.renderMode;
    LOGI("config: Enabled=%d VerboseLog=%d StatsIntervalSec=%d VROutput=%d Screen dist=%.2f width=%.2f height=%.2f MTProtect=%d CamProbe=%ds LensShift=(%.2f,%.2f) RenderMode=%s RestoreCam=%d WorldScale=%.2f HorizonLock=%d Prediction=%.2f ShiftYSign=%.0f FrameMatchOffset=%d Reprojection=%s DebugLayerYaw=%.1f FakeHead=%.1fdeg@%.2fHz",
         g_config.enabled, g_config.verboseLog, g_config.statsIntervalSec, g_config.vrOutput,
         g_config.screenDistance, g_config.screenWidth, g_config.screenHeight, g_config.d3dMultithreadProtect,
         g_config.cameraProbeIntervalSec, g_config.debugLensShiftX, g_config.debugLensShiftY,
         ModeName(g_config.renderMode), g_config.restoreCameraAfterPrepare, g_config.worldScale, g_config.horizonLock, g_config.predictionFrames, g_config.lensShiftYSign, g_config.frameMatchOffset,
         g_config.reprojection ? "Mod" : "Runtime", g_config.debugLayerYawDeg,
         g_config.debugFakeHeadYawDeg, g_config.debugFakeHeadHz);
}

} // namespace so6vr
