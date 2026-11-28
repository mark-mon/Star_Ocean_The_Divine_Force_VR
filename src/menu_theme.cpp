// Look of the vrkit settings menu, matched to the game's own UI.
#include "menu_theme.h"

#pragma clang diagnostic ignored "-Wc23-extensions"   // #embed

namespace so6vr {
namespace {
// The game's English UI font (Arial Nova Condensed) is licensed and packed inside its archives, so the menu uses
// Barlow Semi Condensed (SIL Open Font License, assets/fonts/OFL-Barlow.txt), embedded in the DLL.
const unsigned char kFontMedium[] = {
#embed "../assets/fonts/BarlowSemiCondensed-Medium.ttf"
};
const unsigned char kFontSemiBold[] = {
#embed "../assets/fonts/BarlowSemiCondensed-SemiBold.ttf"
};

} // namespace

// Menu look matched to the game's HUD: dark teal panels, mint-cyan outlines and bars, white condensed text,
// pale-yellow highlights, light key-cap prompts. Colours are IM_COL32 order (0xAABBGGRR).
vrkit::menu::Theme GameMenuTheme() {
    vrkit::menu::Theme t;
    t.tech = true;
    t.text     = 0xFFEBEBE1;   // (225,235,235)
    t.title    = 0xFFFFFFFF;
    t.dim      = 0xFFA8AA7D;   // (125,170,168)
    t.accent   = 0xFFE5F595;   // (149,245,229) the HUD's mint cyan
    t.border   = 0xFFE5F595;
    t.selFill  = 0xF06A5909;   // (9,89,106)
    t.selFill2 = 0x50433604;   // (4,54,67), fading out
    t.warn     = 0xFF99FFFF;   // (255,255,153)
    t.bgTop    = 0xFF1E1805;   // (5,24,30)
    t.bgBottom = 0xFF0F0A02;   // (2,10,15)
    t.keyCap   = 0xFFEEEEE8;
    t.keyText  = 0xFF241E08;
    t.scanline = 0x08E5F595;
    t.chamfer = 40.f;
    t.upperCaseTitle = true; t.upperCaseTabs = true; t.titleSpacing = 2.f;
    t.font = kFontMedium; t.fontBytes = (int)sizeof(kFontMedium);
    t.fontBold = kFontSemiBold; t.fontBoldBytes = (int)sizeof(kFontSemiBold);
    return t;
}

} // namespace so6vr
