// vrkit - renderer-independent part of the in-game settings menu (used by the D3D11 and D3D9 backends).
// Owns the ImGui context, input handling (open/close, navigation) and the menu layout. A backend creates its
// device objects, calls InitImGui once, and each frame: UpdateInput(); if open: <backend>_NewFrame(), BuildFrame(),
// then renders ImGui::GetDrawData().
#pragma once
#include <string>
#include <functional>
#include "vrkit_input.h"

namespace vrkit { namespace menu {

// Look of the menu. The default is vrkit's plain flat panel. A host can match its game's UI: colours (IM_COL32
// order: 0xAABBGGRR), a "tech" panel style (chamfered outline, gradient fills, arrow-shaped tab and row plates,
// key-cap prompts), and TrueType fonts embedded in the host (the data must stay valid; it is not copied or freed).
struct Theme {
    bool tech = false;
    unsigned text = 0xFFF5EEEB, dim = 0xFFAF9E96, accent = 0xFFFFB46E, selFill = 0xFF8C5A3C, warn = 0xFF5ABEFF;
    // tech style only
    unsigned bgTop = 0xF0180F07, bgBottom = 0xF0180F07, border = 0xFFFFB46E, selFill2 = 0x808C5A3C,
             title = 0xFFFFFFFF, keyCap = 0xFFF0F0EB, keyText = 0xFF231E0A, scanline = 0x0AFFFFFF;
    float chamfer = 36.f;           // corner cut of the panel, layout units
    bool upperCaseTitle = false, upperCaseTabs = false;
    const void* font = nullptr;     int fontBytes = 0;       // body text (default: Segoe UI / Arial)
    const void* fontBold = nullptr; int fontBoldBytes = 0;   // title, tabs, key caps (default: the body font)
    float titleSpacing = 0.f;       // extra pixels between title letters
};

struct Settings {
    std::string title = "VR mod";
    unsigned texW = 1280, texH = 960;   // menu size (layout units = texture pixels for D3D11)
    float fontPx = 34.f;                // text size in layout units
    float fontOversample = 1.f;         // D3D9 backend: rasterise the font this much larger (the menu is scaled up when drawn)
    float backgroundAlpha = 0.94f;      // panel opacity (1 = nothing behind the menu shows through)
    std::wstring configPath;            // where Options::Save writes on close (empty: not saved)
    std::function<std::string()> statusLine;   // optional line under the title (fps, version...)
    input::Settings input;
    Theme theme;
};

namespace core {
bool InitImGui(const Settings& s);    // ImGui context, font, style (also configures + hooks input)
void ShutdownImGui();
const Settings& Get();
// Poll input, handle open/close and navigation. Returns true while the menu is open.
bool UpdateInput();
// Build this frame's menu (ImGui::NewFrame ... ImGui::Render). Call after the backend's NewFrame.
void BuildFrame();
bool IsOpen();
void SetOpen(bool open);
} // namespace core

} } // namespace vrkit::menu
