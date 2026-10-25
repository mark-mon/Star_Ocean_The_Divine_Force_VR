#include <windows.h>
#include <string>
#include "log.h"
#include "config.h"
#include "d3d_hooks.h"
#include "camera_probe.h"
#include "render_res.h"
#include "image_quality.h"
#include "rdoc.h"

#define SO6VR_VERSION "0.9.11"

namespace {

// The build we reverse-engineer against (Steam, SO6.exe built 2022-11-25).
constexpr DWORD kKnownExeSize = 36797528;
constexpr DWORD kKnownExeTimestamp = 1669372560;

std::wstring g_gameDir;

bool exeMatches = false;

void LogExeInfo() {
    HMODULE exe = GetModuleHandleW(nullptr);
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(exe);
    auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<BYTE*>(exe) + dos->e_lfanew);
    DWORD ts = nt->FileHeader.TimeDateStamp;

    wchar_t path[MAX_PATH]; GetModuleFileNameW(exe, path, MAX_PATH);
    DWORD size = 0;
    HANDLE h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h != INVALID_HANDLE_VALUE) { size = GetFileSize(h, nullptr); CloseHandle(h); }

    char p8[MAX_PATH * 3]; WideCharToMultiByte(CP_UTF8, 0, path, -1, p8, sizeof(p8), nullptr, nullptr);
    LOGI("exe: %s base=%p size=%lu timestamp=%lu", p8, exe, size, ts);
    if (size != kKnownExeSize || ts != kKnownExeTimestamp)
        LOGW("SO6.exe differs from the build this mod was made for (expected size %lu, timestamp %lu). "
             "The game may have been updated; later stages may need new offsets.", kKnownExeSize, kKnownExeTimestamp);
    else {
        LOGI("exe matches known build");
        exeMatches = true;
    }
}

bool IsTargetProcess() {
    wchar_t path[MAX_PATH]; GetModuleFileNameW(nullptr, path, MAX_PATH);
    const wchar_t* name = wcsrchr(path, L'\\');
    name = name ? name + 1 : path;
    return _wcsicmp(name, L"SO6.exe") == 0;
}

void Startup(HMODULE self) {
    wchar_t path[MAX_PATH]; GetModuleFileNameW(nullptr, path, MAX_PATH);
    g_gameDir = path;
    g_gameDir = g_gameDir.substr(0, g_gameDir.find_last_of(L'\\'));

    so6vr::Log::Open((g_gameDir + L"\\so6vr.log").c_str());
    LOGI("SO6 VR mod %s", SO6VR_VERSION);
    LogExeInfo();
    so6vr::LoadConfig(g_gameDir);

    if (!so6vr::g_config.enabled) { LOGI("Enabled=0 in config.txt; running as plain XInput passthrough"); return; }
    so6vr::RdocInit(g_gameDir);
    if (so6vr::InstallD3DHooks()) LOGI("bootstrap hooks installed");
    if (exeMatches) { so6vr::CameraProbeInstall(); so6vr::RenderResInstall(); so6vr::ImageQualityInstall(); }
    else LOGW("camera probe skipped: game offsets are only valid for the known SO6.exe build");
    (void)self;
}

} // namespace

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(inst);
        // Only instrument the game itself; any other process loading this file just gets XInput passthrough.
        if (IsTargetProcess()) Startup(inst);
        break;
    case DLL_PROCESS_DETACH:
        // reserved != nullptr means the process is exiting: don't touch hooks, other threads are gone.
        if (!reserved) so6vr::RemoveD3DHooks();
        LOGI("unloading");
        so6vr::Log::Close();
        break;
    }
    return TRUE;
}
