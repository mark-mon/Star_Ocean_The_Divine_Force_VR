// Render resolution override: makes the game render at the headset's per-eye resolution (times RenderScale)
// instead of one of its fixed 16:9 "graphics resolution" presets.
//
// SO6.exe (known build):
//   0x479E4  GetGraphicsResolution(int preset, u16* w, u16* h)  - preset table (1280x720 ... 3200x1800)
//   0x477F0  SetGraphicsResolution(SystemConfig* cfg, int preset) - the settings setter; writes the render
//            size block at [0x2541330]: +0x50/+0x54 base size, +0x58/+0x5C base * scale(+0x60).
//            The renderer reallocates its targets when that block changes (same path as the options menu).
#include "render_res.h"
#include "config.h"
#include "hook_util.h"
#include "safe_mem.h"
#include "log.h"
#include <windows.h>
#include <atomic>
#include <cmath>
#include <cstring>

namespace so6vr {
namespace {

constexpr uintptr_t kFnGetRes = 0x479E4;
constexpr uintptr_t kFnSetRes = 0x477F0;
constexpr uintptr_t kResBlockPtr = 0x2541330;
// Mouse: the game keeps cursor positions in "screen units" (its render size, words at [0x248FE00]+0x28/+0x2A)
// and maps them to the window's client area with integer maths. That round trip only loses nothing when
// the render size is an integer multiple of the client size; otherwise the cursor it re-centres every frame
// reads back a pixel off, and the camera turns on its own (and the game flips to mouse/keyboard prompts).
// With a VR render size we make the mapping 1:1 (screen units = client pixels) in both directions.
constexpr uintptr_t kFnSetCursorUnits = 0x3E2BE4;   // void(?, int x, int y): units -> client -> SetCursorPos
constexpr uintptr_t kReadScaleX = 0x13298E;         // movzx eax, word [r10+0x28]  (in the GetCursorPos path)
constexpr uintptr_t kReadScaleY = 0x1329AD;         // movzx eax, word [r10+0x2A]
constexpr uintptr_t kClientRectPtr = 0x248FD58;     // +0x28 HWND, +0x48..+0x54 client rect

using PFN_GetRes = void (*)(int, uint16_t*, uint16_t*);
using PFN_SetRes = void (*)(void*, int);
PFN_GetRes o_GetRes = nullptr;
PFN_SetRes o_SetRes = nullptr;
using PFN_SetCursorUnits = void (*)(void*, int, int);
PFN_SetCursorUnits o_SetCursorUnits = nullptr;
bool g_mouse1to1 = false;

void hk_SetCursorUnits(void* self, int x, int y) {
    if (!g_mouse1to1) { o_SetCursorUnits(self, x, y); return; }
    uintptr_t win = 0;
    if (!SafeLoad(reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)) + kClientRectPtr, win) || !win) { o_SetCursorUnits(self, x, y); return; }
    HWND hwnd = nullptr; int rc[4] = {};
    SafeLoad(win + 0x28, hwnd);
    SafeRead(rc, reinterpret_cast<void*>(win + 0x48), 16);
    POINT pt{x + rc[0], y + rc[1]};
    ClientToScreen(hwnd, &pt);
    SetCursorPos(pt.x, pt.y);
}

// Patch/unpatch the read side: "movzx eax, word [r10+0x28/0x2A]" -> "mov eax, ecx" (ecx = client width/height),
// so the units value equals the client coordinate.
void SetMouse1to1(bool on) {
    if (on == g_mouse1to1) return;
    uintptr_t b = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    static const uint8_t origX[5] = {0x41, 0x0F, 0xB7, 0x42, 0x28}, origY[5] = {0x41, 0x0F, 0xB7, 0x42, 0x2A};
    static const uint8_t patch[5] = {0x89, 0xC8, 0x90, 0x90, 0x90};
    uint8_t* px = reinterpret_cast<uint8_t*>(b + kReadScaleX);
    uint8_t* py = reinterpret_cast<uint8_t*>(b + kReadScaleY);
    const uint8_t* wantX = on ? origX : patch;
    if (memcmp(px, wantX, 5) != 0 || memcmp(py, on ? origY : patch, 5) != 0) { LOGW("mouse mapping: unexpected code bytes, left alone"); return; }
    DWORD old = 0;
    if (!VirtualProtect(px, 0x30, PAGE_EXECUTE_READWRITE, &old)) return;
    memcpy(px, on ? patch : origX, 5);
    memcpy(py, on ? patch : origY, 5);
    VirtualProtect(px, 0x30, old, &old);
    FlushInstructionCache(GetCurrentProcess(), px, 0x30);
    g_mouse1to1 = on;
    LOGI("mouse mapping: %s", on ? "1:1 with the window (VR render size)" : "game default");
}

std::atomic<uint32_t> g_hmdW{0}, g_hmdH{0};   // recommended per-eye size from the OpenXR runtime
std::atomic<void*> g_cfg{nullptr};
std::atomic<int> g_preset{-1};
uint32_t g_wantW = 0, g_wantH = 0;            // what GetRes currently hands out (0 = game preset)
int g_reapplies = 0;

uintptr_t Base() { return reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)); }

// Target render size for the current config, or false to leave the game's preset alone.
bool Desired(uint32_t& w, uint32_t& h) {
    const Config& c = g_config;
    if (c.renderResMode == Config::ResGame) return false;
    if (c.activeMode == Config::Flat && c.renderMode == Config::Flat) return false;
    double bw, bh;
    if (c.renderResMode == Config::ResExplicit) { bw = c.renderResW; bh = c.renderResH; }
    else {
        bw = g_hmdW.load(); bh = g_hmdH.load();
        if (bw <= 0 || bh <= 0) return false;
    }
    double s = c.renderScale > 0.1 ? c.renderScale : 1.0;
    w = uint32_t(std::lround(bw * s / 8.0) * 8);
    h = uint32_t(std::lround(bh * s / 8.0) * 8);
    // Never portrait: with a render image taller than wide the game crashes in its visibility code in some
    // places (e.g. the Hybrid Research Vessel hallways; SO6+0xDD36F). Many headsets recommend portrait per-eye
    // sizes, so widen to a square - same field of view, slightly more horizontal pixels.
    if (w < h) {
        static bool logged = false;
        if (!logged) { logged = true; LOGI("render resolution: %ux%u is portrait; using %ux%u (the game can crash with portrait images)", w, h, h, h); }
        w = h;
    }
    if (w < 640) w = 640;
    if (h < 360) h = 360;
    if (w > 8192) w = 8192;
    if (h > 8192) h = 8192;
    return true;
}

float ResScale() {
    uintptr_t blk = 0; float s = 1.0f;
    if (SafeLoad(Base() + kResBlockPtr, blk) && blk) SafeRead(&s, reinterpret_cast<void*>(blk + 0x60), 4);
    return (s > 0.05f && s < 8.0f) ? s : 1.0f;
}

void hk_GetRes(int preset, uint16_t* w, uint16_t* h) {
    o_GetRes(preset, w, h);
    if (g_wantW && g_wantH && w && h) {
        // The game multiplies the base size by its own scale (+0x60); divide it out so the result is exact.
        float s = ResScale();
        *w = uint16_t(std::lround(g_wantW / s));
        *h = uint16_t(std::lround(g_wantH / s));
    }
}

void hk_SetRes(void* cfg, int preset) {
    g_cfg.store(cfg); g_preset.store(preset);
    uint32_t w, h;
    if (Desired(w, h)) { g_wantW = w; g_wantH = h; } else { g_wantW = g_wantH = 0; }
    o_SetRes(cfg, preset);
    SetMouse1to1(g_wantW != 0 && g_config.mouseFix);
    uintptr_t blk = 0; uint32_t cur[4] = {};
    if (SafeLoad(Base() + kResBlockPtr, blk) && blk) SafeRead(cur, reinterpret_cast<void*>(blk + 0x50), 16);
    LOGI("render resolution: game preset %d -> render %ux%u (base %ux%u)%s", preset, cur[2], cur[3], cur[0], cur[1],
         g_wantW ? " [VR override]" : "");
}

} // namespace

bool RenderResInstall() {
    uintptr_t b = Base();
    bool a = Hook(reinterpret_cast<void*>(b + kFnGetRes), (void*)&hk_GetRes, (void**)&o_GetRes, "GetGraphicsResolution");
    bool c = Hook(reinterpret_cast<void*>(b + kFnSetRes), (void*)&hk_SetRes, (void**)&o_SetRes, "SetGraphicsResolution");
    bool m = Hook(reinterpret_cast<void*>(b + kFnSetCursorUnits), (void*)&hk_SetCursorUnits, (void**)&o_SetCursorUnits, "SetCursorUnits");
    return a && c && m;
}

void RenderResSetHmdRecommended(uint32_t w, uint32_t h) {
    g_hmdW.store(w); g_hmdH.store(h);
    LOGI("headset recommended per-eye size %ux%u (RenderScale %.2f)", w, h, g_config.renderScale);
}

// Game thread, once per frame: re-applies the resolution setting when the wanted size changes
// (headset size known later than the game's startup, mode toggled with F8, ...).
void RenderResTick() {
    void* cfg = g_cfg.load();
    int preset = g_preset.load();
    if (!cfg || preset < 0 || !o_SetRes) return;
    uint32_t w = 0, h = 0;
    if (!Desired(w, h)) { w = h = 0; }
    if (w == g_wantW && h == g_wantH) return;
    if (g_reapplies >= 16) return;   // safety: never loop
    ++g_reapplies;
    LOGI("render resolution: re-applying (want %ux%u)", w, h);
    hk_SetRes(cfg, preset);
}

void RenderResCurrent(uint32_t& w, uint32_t& h) {
    uintptr_t blk = 0; uint32_t cur[2] = {};
    if (SafeLoad(Base() + kResBlockPtr, blk) && blk) SafeRead(cur, reinterpret_cast<void*>(blk + 0x58), 8);
    w = cur[0]; h = cur[1];
}

} // namespace so6vr
