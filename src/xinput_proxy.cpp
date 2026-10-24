// Proxy for xinput9_1_0.dll. The game imports this DLL statically, so placing our
// build next to SO6.exe gets us loaded before the game's own code runs.
// Every export is forwarded to the real system DLL.
#include <windows.h>
#include "log.h"

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

DWORD WINAPI Proxy_XInputGetState(DWORD user, void* state) {
    EnsureReal();
    return p_GetState ? p_GetState(user, state) : ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI Proxy_XInputSetState(DWORD user, void* vib) {
    EnsureReal();
    return p_SetState ? p_SetState(user, vib) : ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI Proxy_XInputGetCapabilities(DWORD user, DWORD flags, void* caps) {
    EnsureReal();
    return p_GetCapabilities ? p_GetCapabilities(user, flags, caps) : ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI Proxy_XInputGetDSoundAudioDeviceGuids(DWORD user, GUID* render, GUID* capture) {
    EnsureReal();
    return p_GetDSoundGuids ? p_GetDSoundGuids(user, render, capture) : ERROR_DEVICE_NOT_CONNECTED;
}

} // extern "C"
