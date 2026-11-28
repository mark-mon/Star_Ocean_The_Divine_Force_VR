#include "vrkit_menu_core.h"
#include "vrkit_options.h"
#include <windows.h>
#include <vector>
#include <algorithm>
#include "imgui.h"
#include "imgui_internal.h"
#include <cctype>
#include <cfloat>

namespace vrkit { namespace menu { namespace core {

namespace {
Settings g_s;
ImGuiContext* g_im = nullptr;
bool g_open = false;
int g_page = 0, g_sel = 0;
LARGE_INTEGER g_qpf, g_last;
ImFont *g_fBody = nullptr, *g_fTitle = nullptr, *g_fTab = nullptr, *g_fKey = nullptr;

std::vector<int> PageItems(int page) {
    auto& opts = Options::Get().All();
    auto pages = Options::Get().Pages();
    std::vector<int> r;
    if (pages.empty()) return r;
    page = (std::min)(page, (int)pages.size() - 1);
    for (int k = 0; k < (int)opts.size(); ++k)
        if (opts[k].page == pages[page] && (!opts[k].visible || opts[k].visible())) r.push_back(k);
    return r;
}

void Close() {
    g_open = false;
    input::SetBlocking(false);
    if (Options::Get().Dirty() && !g_s.configPath.empty()) {
        if (Options::Get().Save(g_s.configPath)) Log("vrkit: settings saved");
    }
}

void Navigate(uint32_t b) {
    auto pages = Options::Get().Pages();
    if (pages.empty()) return;
    if (b & input::PagePrev) { g_page = (g_page + (int)pages.size() - 1) % (int)pages.size(); g_sel = 0; }
    if (b & input::PageNext) { g_page = (g_page + 1) % (int)pages.size(); g_sel = 0; }
    auto items = PageItems(g_page);
    if (items.empty()) return;
    int n = (int)items.size();
    if (b & input::Up) g_sel = (g_sel + n - 1) % n;
    if (b & input::Down) g_sel = (g_sel + 1) % n;
    g_sel = (std::min)(g_sel, n - 1);
    Option& o = Options::Get().All()[items[g_sel]];
    if (b & input::Left) Options::Get().Step(o, -1);
    if (b & input::Right) Options::Get().Step(o, +1);
    if (b & input::Accept) Options::Get().Activate(o);
    if (b & input::Reset) Options::Get().Reset(o);
}
} // namespace

bool InitImGui(const Settings& s) {
    if (g_im) return true;
    g_s = s;
    input::Configure(s.input);
    input::InstallHooks();
    IMGUI_CHECKVERSION();
    g_im = ImGui::CreateContext();
    ImGui::SetCurrentContext(g_im);
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; io.LogFilename = nullptr;
    io.DisplaySize = ImVec2((float)s.texW, (float)s.texH);
    // the theme's embedded font, else a clear UI font from Windows at the requested size; ImGui's built-in font as fallback
    float os = s.fontOversample > 0 ? s.fontOversample : 1.f;
    const Theme& T = s.theme;
    auto fromMemory = [&](const void* data, int bytes, float px, float spacing) -> ImFont* {
        if (!data || bytes <= 0) return nullptr;
        ImFontConfig fc; fc.FontDataOwnedByAtlas = false; fc.OversampleH = 2; fc.GlyphExtraSpacing.x = spacing * os;
        return io.Fonts->AddFontFromMemoryTTF(const_cast<void*>(data), bytes, px * os, &fc);
    };
    ImFont* f = fromMemory(T.font, T.fontBytes, s.fontPx, 0.f);
    wchar_t win[MAX_PATH]; char path[MAX_PATH * 2];
    if (!f && GetWindowsDirectoryW(win, MAX_PATH)) {
        for (const wchar_t* name : { L"\\Fonts\\segoeui.ttf", L"\\Fonts\\arial.ttf" }) {
            std::wstring p = std::wstring(win) + name;
            WideCharToMultiByte(CP_UTF8, 0, p.c_str(), -1, path, sizeof(path), nullptr, nullptr);
            if (GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES && (f = io.Fonts->AddFontFromFileTTF(path, s.fontPx * os))) break;
        }
    }
    if (!f) { ImFontConfig fc; fc.SizePixels = s.fontPx * os; f = io.Fonts->AddFontDefault(&fc); }
    g_fBody = f;
    g_fTitle = fromMemory(T.fontBold ? T.fontBold : T.font, T.fontBold ? T.fontBoldBytes : T.fontBytes, s.fontPx * 1.08f, T.titleSpacing);
    g_fTab = fromMemory(T.fontBold ? T.fontBold : T.font, T.fontBold ? T.fontBoldBytes : T.fontBytes, s.fontPx, 0.f);
    g_fKey = fromMemory(T.fontBold ? T.fontBold : T.font, T.fontBold ? T.fontBoldBytes : T.fontBytes, s.fontPx * 0.62f, 0.f);
    if (!g_fTitle) g_fTitle = f;
    if (!g_fTab) g_fTab = f;
    if (!g_fKey) g_fKey = f;
    io.FontDefault = f;
    io.FontGlobalScale = 1.f / os;
    ImGuiStyle& st = ImGui::GetStyle();
    st.WindowRounding = 14.f; st.WindowBorderSize = 0.f; st.WindowPadding = ImVec2(0, 0);
    st.Colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.08f, 0.11f, T.tech ? 0.f : s.backgroundAlpha);   // tech: BuildFrameTech draws the panel
    QueryPerformanceFrequency(&g_qpf); QueryPerformanceCounter(&g_last);
    return true;
}

void ShutdownImGui() {
    if (g_im) { ImGui::DestroyContext(g_im); g_im = nullptr; }
    g_open = false;
}

const Settings& Get() { return g_s; }

bool UpdateInput() {
    if (!g_im) return false;
    static int rehook = 0;
    if ((++rehook % 300) == 1) input::InstallHooks();   // the game may load XInput late
    uint32_t b = input::Poll();
    if (b & input::Toggle) { if (g_open) Close(); else { g_open = true; input::SetBlocking(true); } b &= ~input::Toggle; }
    else if (g_open && (b & input::Back)) { Close(); return false; }
    if (!g_open) return false;
    Navigate(b);
    ImGui::SetCurrentContext(g_im);
    return true;
}

void BuildFrameTech();

void BuildFrame() {
    ImGui::SetCurrentContext(g_im);
    if (g_s.theme.tech) { BuildFrameTech(); return; }
    ImGuiIO& io = ImGui::GetIO();
    LARGE_INTEGER now; QueryPerformanceCounter(&now);
    io.DeltaTime = (std::max)(0.001f, (float)(now.QuadPart - g_last.QuadPart) / (float)g_qpf.QuadPart); g_last = now;
    io.DisplaySize = ImVec2((float)g_s.texW, (float)g_s.texH);
    ImGui::NewFrame();
    const float W = (float)g_s.texW, H = (float)g_s.texH, pad = g_s.fontPx * 0.6f;
    ImGui::SetNextWindowPos(ImVec2(0, 0)); ImGui::SetNextWindowSize(ImVec2(W, H));
    ImGui::Begin("##vrkit", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollbar);
    const Theme& T = g_s.theme;
    const ImU32 cText = T.text, cDim = T.dim, cAcc = T.accent, cSel = T.selFill, cWarn = T.warn;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float y = pad;
    // title + status
    dl->AddText(ImVec2(pad, y), cText, g_s.title.c_str());
    if (g_s.statusLine) { std::string st = g_s.statusLine(); ImVec2 ts = ImGui::CalcTextSize(st.c_str()); dl->AddText(ImVec2(W - pad - ts.x, y), cDim, st.c_str()); }
    y += g_s.fontPx * 1.4f;
    // page tabs
    auto pages = Options::Get().Pages();
    float x = pad;
    for (int p = 0; p < (int)pages.size(); ++p) {
        ImVec2 ts = ImGui::CalcTextSize(pages[p].c_str());
        if (p == g_page) dl->AddRectFilled(ImVec2(x - 8, y - 4), ImVec2(x + ts.x + 8, y + ts.y + 4), cSel, 6.f);
        dl->AddText(ImVec2(x, y), p == g_page ? cText : cDim, pages[p].c_str());
        x += ts.x + 28;
    }
    y += g_s.fontPx * 1.5f;
    dl->AddLine(ImVec2(pad, y - 8), ImVec2(W - pad, y - 8), cDim, 2.f);
    // rows (scrolled so the selection stays visible)
    auto items = PageItems(g_page);
    float rowH = g_s.fontPx * 1.35f, listTop = y, listBottom = H - g_s.fontPx * 5.2f;
    int visibleRows = (std::max)(1, (int)((listBottom - listTop) / rowH));
    int first = (std::max)(0, (std::min)(g_sel - visibleRows / 2, (int)items.size() - visibleRows));
    auto& opts = Options::Get().All();
    for (int r = first; r < (int)items.size() && r < first + visibleRows; ++r) {
        Option& o = opts[items[r]];
        float ry = listTop + (r - first) * rowH;
        if (r == g_sel) dl->AddRectFilled(ImVec2(pad * 0.5f, ry - 4), ImVec2(W - pad * 0.5f, ry + rowH - 6), cSel, 6.f);
        dl->AddText(ImVec2(pad, ry), cText, o.label.c_str());
        std::string v = Options::Get().DisplayText(o);
        bool adjustable = o.type != OptType::Action;
        std::string shown = o.type == OptType::Action ? "[ run ]" : (r == g_sel && adjustable ? "<  " + v + "  >" : v);
        ImVec2 vs = ImGui::CalcTextSize(shown.c_str());
        float vx = W - pad - vs.x;
        if (o.needsRestart) {
            const char* rs = "restart";
            ImVec2 rsz = ImGui::CalcTextSize(rs);
            dl->AddText(ImVec2(vx - rsz.x - 24, ry), cWarn, rs);
        }
        dl->AddText(ImVec2(vx, ry), r == g_sel ? cAcc : cText, shown.c_str());
    }
    if (items.empty()) dl->AddText(ImVec2(pad, listTop), cDim, "(nothing on this page)");
    // help for the selected option + controls
    float hy = H - g_s.fontPx * 4.6f;
    dl->AddLine(ImVec2(pad, hy - 10), ImVec2(W - pad, hy - 10), cDim, 2.f);
    if (!items.empty()) {
        const Option& o = opts[items[(std::min)(g_sel, (int)items.size() - 1)]];
        std::string h = o.help;
        if (!o.key.empty()) h += (h.empty() ? "" : "   ") + std::string("[") + o.key + "]";
        ImGui::SetCursorScreenPos(ImVec2(pad, hy));
        ImGui::PushTextWrapPos(W - pad);
        ImGui::PushStyleColor(ImGuiCol_Text, cDim);
        ImGui::TextWrapped("%s", h.c_str());
        ImGui::PopStyleColor(); ImGui::PopTextWrapPos();
    }
    const char* keys = "A: change   Y: undo   LB/RB: page   B: close";
    dl->AddText(ImVec2(pad, H - g_s.fontPx * 1.4f), cDim, keys);
    ImGui::End();
    ImGui::Render();
}

// ---- "tech" theme: chamfered panel, gradient plates, key-cap prompts --------------------------------------------
namespace {
ImU32 Alpha(ImU32 c, float a) {   // scale a colour's alpha
    unsigned al = (unsigned)((c >> IM_COL32_A_SHIFT) & 0xFF);
    return (c & ~IM_COL32_A_MASK) | ((ImU32)(al * (std::min)(1.f, (std::max)(0.f, a)) + 0.5f) << IM_COL32_A_SHIFT);
}
ImU32 Lerp(ImU32 a, ImU32 b, float t) {
    ImVec4 x = ImGui::ColorConvertU32ToFloat4(a), y = ImGui::ColorConvertU32ToFloat4(b);
    return ImGui::ColorConvertFloat4ToU32(ImVec4(x.x + (y.x - x.x) * t, x.y + (y.y - x.y) * t, x.z + (y.z - x.z) * t, x.w + (y.w - x.w) * t));
}
// Convex polygon, colour graded along x (horizontal) or y (vertical) between c0 (at lo) and c1 (at hi).
void FillGradient(ImDrawList* dl, const ImVec2* p, int n, ImU32 c0, ImU32 c1, bool horizontal, float lo, float hi) {
    ImVec2 uv = dl->_Data->TexUvWhitePixel;
    dl->PrimReserve((n - 2) * 3, n);
    unsigned base = dl->_VtxCurrentIdx;
    for (int i = 0; i < n; ++i) {
        float t = ((horizontal ? p[i].x : p[i].y) - lo) / (hi - lo > 1e-3f ? hi - lo : 1.f);
        dl->PrimWriteVtx(p[i], uv, Lerp(c0, c1, t));
    }
    for (int i = 1; i < n - 1; ++i) { dl->PrimWriteIdx((ImDrawIdx)base); dl->PrimWriteIdx((ImDrawIdx)(base + i)); dl->PrimWriteIdx((ImDrawIdx)(base + i + 1)); }
}
// Arrow-ended plate: both ends pointed (tabs) or square-left / pointed-right (rows).
int Plate(ImVec2* p, float x0, float y0, float x1, float y1, float tipL, float tipR) {
    float ym = (y0 + y1) * 0.5f; int n = 0;
    p[n++] = ImVec2(x0 + tipL, y0); p[n++] = ImVec2(x1 - tipR, y0); p[n++] = ImVec2(x1, ym);
    p[n++] = ImVec2(x1 - tipR, y1); p[n++] = ImVec2(x0 + tipL, y1);
    if (tipL > 0) p[n++] = ImVec2(x0, ym); else { p[4] = ImVec2(x0, y1); p[0] = ImVec2(x0, y0); }
    return n;
}
float FSize(ImFont* f) { return f->FontSize * ImGui::GetIO().FontGlobalScale; }
ImVec2 Measure(ImFont* f, const char* s) { return f->CalcTextSizeA(FSize(f), FLT_MAX, 0.f, s); }
void Text(ImDrawList* dl, ImFont* f, ImVec2 at, ImU32 c, const char* s) { dl->AddText(f, FSize(f), at, c, s); }
std::string Upper(std::string s) { for (char& ch : s) ch = (char)toupper((unsigned char)ch); return s; }
// Key-cap prompt like the game's: a light rounded square with the button name in dark text. Returns its width.
float KeyCap(ImDrawList* dl, ImVec2 at, float h, const char* key, const Theme& T) {
    ImVec2 ts = Measure(g_fKey, key);
    float w = (std::max)(h, ts.x + h * 0.45f);
    dl->AddRectFilled(at, ImVec2(at.x + w, at.y + h), T.keyCap, h * 0.18f);
    Text(dl, g_fKey, ImVec2(at.x + (w - ts.x) * 0.5f, at.y + (h - ts.y) * 0.5f), T.keyText, key);
    return w;
}
void Triangle(ImDrawList* dl, float x, float cy, float s, bool right, ImU32 c) {
    if (right) dl->AddTriangleFilled(ImVec2(x, cy - s), ImVec2(x + s * 0.9f, cy), ImVec2(x, cy + s), c);
    else dl->AddTriangleFilled(ImVec2(x + s * 0.9f, cy - s), ImVec2(x, cy), ImVec2(x + s * 0.9f, cy + s), c);
}
} // namespace

void BuildFrameTech() {
    const Theme& T = g_s.theme;
    ImGuiIO& io = ImGui::GetIO();
    LARGE_INTEGER now; QueryPerformanceCounter(&now);
    io.DeltaTime = (std::max)(0.001f, (float)(now.QuadPart - g_last.QuadPart) / (float)g_qpf.QuadPart); g_last = now;
    io.DisplaySize = ImVec2((float)g_s.texW, (float)g_s.texH);
    ImGui::NewFrame();
    const float W = (float)g_s.texW, H = (float)g_s.texH, F = g_s.fontPx, pad = F * 0.9f, ch = T.chamfer, m = 3.f;
    ImGui::SetNextWindowPos(ImVec2(0, 0)); ImGui::SetNextWindowSize(ImVec2(W, H));
    ImGui::Begin("##vrkit", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float bgA = g_s.backgroundAlpha;

    // panel: top-left and bottom-right corners cut, vertical gradient, faint scan lines, cyan outline
    ImVec2 panel[6] = { ImVec2(m + ch, m), ImVec2(W - m, m), ImVec2(W - m, H - m - ch), ImVec2(W - m - ch, H - m), ImVec2(m, H - m), ImVec2(m, m + ch) };
    FillGradient(dl, panel, 6, Alpha(T.bgTop, bgA), Alpha(T.bgBottom, bgA), false, 0.f, H);
    for (float y = m + 2; y < H - m; y += 4.f) {
        float x0 = m + (std::max)(0.f, ch - (y - m)), x1 = W - m - (std::max)(0.f, y - (H - m - ch));
        dl->AddLine(ImVec2(x0, y), ImVec2(x1, y), T.scanline, 1.f);
    }
    dl->AddPolyline(panel, 6, Alpha(T.border, 0.12f), ImDrawFlags_Closed, 7.f);   // soft glow
    dl->AddPolyline(panel, 6, Alpha(T.border, 0.55f), ImDrawFlags_Closed, 2.f);
    // brighter accents along the cut corners
    dl->AddLine(ImVec2(m, m + ch + F * 1.2f), ImVec2(m, m + ch), T.border, 4.f);
    dl->AddLine(ImVec2(m, m + ch), ImVec2(m + ch, m), T.border, 4.f);
    dl->AddLine(ImVec2(m + ch, m), ImVec2(m + ch + F * 2.4f, m), T.border, 4.f);
    dl->AddLine(ImVec2(W - m, H - m - ch - F * 1.2f), ImVec2(W - m, H - m - ch), T.border, 4.f);
    dl->AddLine(ImVec2(W - m, H - m - ch), ImVec2(W - m - ch, H - m), T.border, 4.f);
    dl->AddLine(ImVec2(W - m - ch, H - m), ImVec2(W - m - ch - F * 2.4f, H - m), T.border, 4.f);

    // header: diamond mark, title, status
    float y = pad * 0.85f;
    float tH = FSize(g_fTitle), dcx = pad + tH * 0.32f, dcy = y + tH * 0.52f, dr = tH * 0.32f;
    dl->AddQuad(ImVec2(dcx, dcy - dr), ImVec2(dcx + dr, dcy), ImVec2(dcx, dcy + dr), ImVec2(dcx - dr, dcy), T.border, 2.5f);
    dl->AddQuadFilled(ImVec2(dcx, dcy - dr * 0.45f), ImVec2(dcx + dr * 0.45f, dcy), ImVec2(dcx, dcy + dr * 0.45f), ImVec2(dcx - dr * 0.45f, dcy), T.accent);
    std::string title = T.upperCaseTitle ? Upper(g_s.title) : g_s.title;
    Text(dl, g_fTitle, ImVec2(pad + tH * 0.85f, y), T.title, title.c_str());
    if (g_s.statusLine) {
        std::string st = g_s.statusLine(); ImVec2 ts = Measure(g_fBody, st.c_str());
        Text(dl, g_fBody, ImVec2(W - pad - ts.x, y + (tH - ts.y) * 0.5f), T.dim, st.c_str());
    }
    y += tH + F * 0.35f;
    dl->AddRectFilledMultiColor(ImVec2(pad, y), ImVec2(W - pad, y + 2), T.border, Alpha(T.border, 0.f), Alpha(T.border, 0.f), T.border);
    y += F * 0.55f;

    // page tabs: LB [tab] [tab] ... RB, the current one on an arrow-ended plate
    auto pages = Options::Get().Pages();
    float kh = F * 0.95f, tabH = F * 1.35f, tip = tabH * 0.32f;
    float x = pad;
    x += KeyCap(dl, ImVec2(x, y + (tabH - kh) * 0.5f), kh, "LB", T) + F * 0.45f;
    for (int p = 0; p < (int)pages.size(); ++p) {
        std::string name = T.upperCaseTabs ? Upper(pages[p]) : pages[p];
        ImVec2 ts = Measure(g_fTab, name.c_str());
        float w = ts.x + tip * 2 + F * 0.7f;
        if (p == g_page) {
            ImVec2 pl[6]; int n = Plate(pl, x, y, x + w, y + tabH, tip, tip);
            FillGradient(dl, pl, n, T.selFill, T.selFill2, false, y, y + tabH);
            dl->AddPolyline(pl, n, T.border, ImDrawFlags_Closed, 2.f);
        }
        Text(dl, g_fTab, ImVec2(x + (w - ts.x) * 0.5f, y + (tabH - ts.y) * 0.5f), p == g_page ? T.title : T.dim, name.c_str());
        x += w + F * 0.05f;
    }
    KeyCap(dl, ImVec2(x + F * 0.4f, y + (tabH - kh) * 0.5f), kh, "RB", T);
    y += tabH + F * 0.6f;

    // option rows (scrolled so the selection stays visible)
    auto items = PageItems(g_page);
    float rowH = F * 1.38f, listTop = y, listBottom = H - F * 5.4f;
    int visibleRows = (std::max)(1, (int)((listBottom - listTop) / rowH));
    int first = (std::max)(0, (std::min)(g_sel - visibleRows / 2, (int)items.size() - visibleRows));
    auto& opts = Options::Get().All();
    float rx0 = pad * 0.6f, rx1 = W - pad * 0.6f - (items.size() > (size_t)visibleRows ? F * 0.5f : 0.f);
    for (int r = first; r < (int)items.size() && r < first + visibleRows; ++r) {
        Option& o = opts[items[r]];
        float ry = listTop + (r - first) * rowH, ty = ry + (rowH - FSize(g_fBody)) * 0.5f - 1.f;
        bool sel = r == g_sel;
        if (sel) {
            ImVec2 pl[6]; int n = Plate(pl, rx0, ry + 1, rx1, ry + rowH - 1, 0.f, rowH * 0.38f);
            FillGradient(dl, pl, n, T.selFill, T.selFill2, true, rx0, rx1);
            dl->AddPolyline(pl, n, Alpha(T.border, 0.7f), ImDrawFlags_Closed, 1.5f);
            dl->AddRectFilled(ImVec2(rx0, ry + 1), ImVec2(rx0 + 6, ry + rowH - 1), T.border);
        } else {
            dl->AddLine(ImVec2(rx0 + F * 0.3f, ry + rowH - 1), ImVec2(rx1 - F * 0.3f, ry + rowH - 1), Alpha(T.border, 0.13f), 1.f);
        }
        Text(dl, g_fBody, ImVec2(pad, ty), sel ? T.title : T.text, o.label.c_str());
        const float arrow = F * 0.26f, cy = ry + rowH * 0.5f;
        float vr = rx1 - rowH * 0.5f - arrow - F * 0.4f;   // right edge of the value text (room for the arrow and the plate's tip)
        float vx;
        if (o.type == OptType::Action) {
            const char* run = "Run"; ImVec2 ts = Measure(g_fBody, run);
            vx = vr - ts.x;
            Text(dl, g_fBody, ImVec2(vx, ty), sel ? T.accent : T.dim, run);
            if (sel) vx -= KeyCap(dl, ImVec2(vx - kh * 1.15f, ry + (rowH - kh) * 0.5f), kh, "A", T) + kh * 0.15f;
        } else {
            std::string v = Options::Get().DisplayText(o);
            ImVec2 vs = Measure(g_fBody, v.c_str());
            vx = vr - vs.x;
            Text(dl, g_fBody, ImVec2(vx, ty), sel ? T.accent : T.text, v.c_str());
            if (sel) {
                Triangle(dl, vr + F * 0.4f, cy, arrow, true, T.accent);
                vx -= F * 0.4f + arrow;
                Triangle(dl, vx, cy, arrow, false, T.accent);
            }
        }
        if (o.needsRestart) {
            const char* rs = "RESTART"; ImVec2 rsz = Measure(g_fKey, rs);
            float bx = vx - rsz.x - F * 0.9f, bh = rsz.y + 6;
            dl->AddRect(ImVec2(bx - 6, ry + (rowH - bh) * 0.5f), ImVec2(bx + rsz.x + 6, ry + (rowH + bh) * 0.5f), Alpha(T.warn, 0.8f), 3.f, 0, 1.5f);
            Text(dl, g_fKey, ImVec2(bx, ry + (rowH - rsz.y) * 0.5f), T.warn, rs);
        }
    }
    if (items.empty()) Text(dl, g_fBody, ImVec2(pad, listTop), T.dim, "(nothing on this page)");
    if (items.size() > (size_t)visibleRows) {   // scroll bar
        float sx = W - pad * 0.6f - F * 0.15f, top = listTop + 2, bot = listTop + visibleRows * rowH - 2;
        float t0 = top + (bot - top) * first / items.size(), t1 = top + (bot - top) * (first + visibleRows) / items.size();
        dl->AddLine(ImVec2(sx, top), ImVec2(sx, bot), Alpha(T.border, 0.2f), 2.f);
        dl->AddRectFilled(ImVec2(sx - 2.5f, t0), ImVec2(sx + 2.5f, t1), T.border, 2.f);
    }

    // help for the selected option
    float hy = H - F * 4.7f;
    dl->AddRectFilledMultiColor(ImVec2(pad, hy - 12), ImVec2(W - pad, hy - 10), Alpha(T.border, 0.f), T.border, T.border, Alpha(T.border, 0.f));
    if (!items.empty()) {
        const Option& o = opts[items[(std::min)(g_sel, (int)items.size() - 1)]];
        std::string h = o.help;
        if (!o.key.empty()) h += (h.empty() ? "" : "   ") + std::string("[") + o.key + "]";
        ImGui::SetCursorScreenPos(ImVec2(pad, hy));
        ImGui::PushTextWrapPos(W - pad);
        ImGui::PushStyleColor(ImGuiCol_Text, T.dim);
        ImGui::TextWrapped("%s", h.c_str());
        ImGui::PopStyleColor(); ImGui::PopTextWrapPos();
    }
    // control prompts
    float fy = H - F * 1.55f;
    x = pad + ch * 0.4f;
    const char* prompts[][2] = { { "A", "Change" }, { "Y", "Undo" }, { "B", "Close" } };
    for (auto& pr : prompts) {
        x += KeyCap(dl, ImVec2(x, fy + (FSize(g_fBody) - kh) * 0.5f), kh, pr[0], T) + F * 0.3f;
        Text(dl, g_fBody, ImVec2(x, fy), T.text, pr[1]);
        x += Measure(g_fBody, pr[1]).x + F * 0.9f;
    }
    ImGui::End();
    ImGui::Render();
}

bool IsOpen() { return g_open; }
void SetOpen(bool open) { if (open == g_open) return; if (open) { g_open = true; input::SetBlocking(true); } else Close(); }

} } } // namespace vrkit::menu::core
