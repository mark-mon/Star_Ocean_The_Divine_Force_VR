// Proxy for xinput9_1_0.dll. The game imports this DLL statically, so placing our
// build next to SO6.exe gets us loaded before the game's own code runs.
// Every export is forwarded to the real system DLL.
#include <windows.h>
#include "log.h"
#include "config.h"
#include <atomic>

namespace {

using PFN_GetState        = DWORD (WINAPI*)(DWORD, void*);
using PFN_SetState        = DWORD (WINAPI*)(DWORD, void*);
using PFN_GetCapabilities = DWORD (WINAPI*)(DWORD, DWORD, void*);
using PFN_GetDSoundGuids  = DWORD (WINAPI*)(DWORD, GUID*, GUID*);

HMODULE             g_real = nullptr;
PFN_GetState        p_GetState = nullptr;
PFN_SetState        p_SetState = nullptr;
PFN_GetCapabilities p_GetCapabilities = nullptr;
PFN_GetDSoundGuids  p_GetDSoundGuids = nullptr;
INIT_ONCE           g_once = INIT_ONCE_STATIC_INIT;

BOOL CALLBACK LoadReal(PINIT_ONCE, PVOID, PVOID*) {
    wchar_t path[MAX_PATH];
    UINT n = GetSystemDirectoryW(path, MAX_PATH);
    if (n == 0 || n > MAX_PATH - 32) return TRUE;
    wcscat_s(path, L"\\xinput9_1_0.dll");
    g_real = LoadLibraryW(path);
    if (!g_real) { LOGE("failed to load system xinput9_1_0.dll (err %lu)", GetLastError()); return TRUE; }
    p_GetState        = (PFN_GetState)GetProcAddress(g_real, "XInputGetState");
    p_SetState        = (PFN_SetState)GetProcAddress(g_real, "XInputSetState");
    p_GetCapabilities = (PFN_GetCapabilities)GetProcAddress(g_real, "XInputGetCapabilities");
    p_GetDSoundGuids  = (PFN_GetDSoundGuids)GetProcAddress(g_real, "XInputGetDSoundAudioDeviceGuids");
    LOGI("system xinput9_1_0.dll loaded");
    return TRUE;
}

inline void EnsureReal() { InitOnceExecuteOnce(&g_once, LoadReal, nullptr, nullptr); }

} // namespace

extern "C" {

// Diagnostic (DebugFakePad): pad 0 reports keyboard-driven buttons, so automated tests can press gamepad-only
// actions. F5 = RB, F6 = LB, F7 = RT, Numpad 8/2/4/6 = left stick.
static std::atomic<unsigned> s_getStateCalls{0};
static bool s_fakeArmed = false;   // the fake pad appears only after F5 is first pressed (menus stay keyboard)
DWORD WINAPI Proxy_XInputGetState(DWORD user, void* state) {
    EnsureReal();
    DWORD r = p_GetState ? p_GetState(user, state) : ERROR_DEVICE_NOT_CONNECTED;
    if (so6vr::g_config.debugFakePad && !s_fakeArmed && (GetAsyncKeyState(VK_F5) & 0x8000)) s_fakeArmed = true;
    if (so6vr::g_config.debugFakePad && s_fakeArmed && user == 0 && state) {
        unsigned n = ++s_getStateCalls;
        if (n == 1 || (n % 3000) == 0) LOGI("fake pad: XInputGetState calls %u (real result %lu)", n, r);
        static DWORD packet = 0;
        uint8_t* st = static_cast<uint8_t*>(state);
        if (r != ERROR_SUCCESS) memset(st, 0, 16);
        *reinterpret_cast<DWORD*>(st) = ++packet;
        WORD& b = *reinterpret_cast<WORD*>(st + 4);
        auto down = [](int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; };
        if (down(VK_F5)) b |= 0x0200;
        if (down(VK_F6)) b |= 0x0100;
        if (down(VK_F7)) st[7] = 255;
        short& lx = *reinterpret_cast<short*>(st + 8); short& ly = *reinterpret_cast<short*>(st + 10);
        if (down(VK_NUMPAD8)) ly = 32767; if (down(VK_NUMPAD2)) ly = -32767;
        if (down(VK_NUMPAD4)) lx = -32767; if (down(VK_NUMPAD6)) lx = 32767;
        r = ERROR_SUCCESS;
    }
    return r;
}

DWORD WINAPI Proxy_XInputSetState(DWORD user, void* vib) {
    EnsureReal();
    return p_SetState ? p_SetState(user, vib) : ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI Proxy_XInputGetCapabilities(DWORD user, DWORD flags, void* caps) {
    EnsureReal();
    DWORD r = p_GetCapabilities ? p_GetCapabilities(user, flags, caps) : ERROR_DEVICE_NOT_CONNECTED;
    if (so6vr::g_config.debugFakePad && s_fakeArmed && user == 0 && caps && r != ERROR_SUCCESS) {
        memset(caps, 0, 20); static_cast<uint8_t*>(caps)[0] = 1; static_cast<uint8_t*>(caps)[1] = 1;   // gamepad
        *reinterpret_cast<WORD*>(static_cast<uint8_t*>(caps) + 4) = 0xF3FF;
        r = ERROR_SUCCESS;
    }
    return r;
}

DWORD WINAPI Proxy_XInputGetDSoundAudioDeviceGuids(DWORD user, GUID* render, GUID* capture) {
    EnsureReal();
    return p_GetDSoundGuids ? p_GetDSoundGuids(user, render, capture) : ERROR_DEVICE_NOT_CONNECTED;
}

} // extern "C"
