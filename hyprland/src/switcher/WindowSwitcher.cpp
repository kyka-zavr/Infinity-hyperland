// Copyright (c) 2026, kyka-zavr.

#include "WindowSwitcher.hpp"
#include "AppInfo.hpp"

#include "../Compositor.hpp"
#include "../config/ConfigValue.hpp"
#include "../config/shared/actions/ConfigActions.hpp"
#include "../desktop/history/WindowHistoryTracker.hpp"
#include "../desktop/state/FocusState.hpp"
#include "../desktop/view/window/Window.hpp"
#include "../devices/IKeyboard.hpp"
#include "../input/Keys.hpp"
#include "../layout/LayoutManager.hpp"
#include "../managers/input/InputManager.hpp"
#include "../output/Monitor.hpp"
#include "../state/MonitorState.hpp"
#include "../render/Renderer.hpp"
#include "../render/pass/RectPassElement.hpp"
#include "../render/pass/TexPassElement.hpp"
#include "../desktop/view/WLSurface.hpp"
#include "../protocols/core/Compositor.hpp"

#include <hyprgraphics/image/Image.hpp>

#include <algorithm>
#include <cmath>
#include <ranges>

using namespace Switcher;

// logical pixels, multiplied by the monitor scale when drawn
static constexpr float TILE_W = 280.F;
static constexpr float TILE_H = 226.F;
static constexpr float GAP    = 12.F;
static constexpr float PAD    = 20.F;
static constexpr float SELPAD = 3.F;

// animations, milliseconds
static constexpr float OPEN_MS  = 150.F;
static constexpr float CLOSE_MS = 110.F;

// 0 at the start, 1 at the end of the opening or closing animation
static float animationProgress(const std::chrono::steady_clock::time_point& start, float durationMs, bool backwards) {
    const float T = std::clamp(std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - start).count() / durationMs, 0.F, 1.F);
    return backwards ? 1.F - T : T;
}

UP<CWindowSwitcher>& Switcher::switcher() {
    static UP<CWindowSwitcher> p = makeUnique<CWindowSwitcher>();
    return p;
}

bool CWindowSwitcher::active() const {
    return !m_entries.empty() && !m_closing;
}

std::string CWindowSwitcher::trigger(bool backwards) {
    if (active()) {
        step(backwards);
        return {};
    }

    // a new press while the panel is fading out starts a fresh one
    m_entries.clear();
    m_layouts.clear();
    m_closing = false;

    const auto CURRENT = Desktop::focusState()->window();

    std::vector<PHLWINDOW> windows; // most recently used first
    if (CURRENT && validMapped(CURRENT))
        windows.emplace_back(CURRENT);

    for (const auto& ref : Desktop::History::windowTracker()->fullHistory() | std::views::reverse) {
        const auto W = ref.lock();
        if (!W || !validMapped(W) || W->isHidden() || std::ranges::contains(windows, W))
            continue;

        windows.emplace_back(W);
    }

    if (windows.size() < 2)
        return "Alt-Tab: no other window";

    m_entries.clear();
    for (const auto& w : windows) {
        const auto& APP = lookupApp(w->metadata().appID().empty() ? w->metadata().initialAppID() : w->metadata().appID());
        m_entries.emplace_back(SEntry{.window = w, .appName = APP.name, .iconPath = APP.iconPath});
    }

    m_selected = backwards ? m_entries.size() - 1 : 1;
    m_openPos  = g_pInputManager->getMouseCoordsInternal();

    // the panel belongs to the monitor the pointer is on
    m_monitor = State::monitorState()->query().vec(m_openPos).run();
    if (!m_monitor)
        m_monitor = Desktop::focusState()->monitor();

    m_openTime     = std::chrono::steady_clock::now();
    m_animLast     = m_openTime;
    m_selAnimValid = false;
    m_layouts.clear();

    // releasing one of the modifiers that were held when the switcher opened confirms the choice (Shift only changes direction)
    m_holdMods = sc<uint32_t>(g_pInputManager->getModsFromAllKBs()) & (sc<uint32_t>(Input::HL_MODIFIER_ALT) | sc<uint32_t>(Input::HL_MODIFIER_CTRL) | sc<uint32_t>(Input::HL_MODIFIER_META));

    redraw();
    return {};
}

void CWindowSwitcher::step(bool backwards) {
    if (m_entries.empty())
        return;

    const auto N = m_entries.size();
    m_selected   = backwards ? (m_selected + N - 1) % N : (m_selected + 1) % N;
    redraw();
}

void CWindowSwitcher::redraw() {
    for (const auto& m : State::monitorState()->monitors()) {
        g_pHyprRenderer->damageMonitor(m);
        m->scheduleFrame();
    }
}

// closing is animated: the panel fades out and then finish() clears it
void CWindowSwitcher::close() {
    if (m_entries.empty())
        return;

    m_closing   = true;
    m_closeTime = std::chrono::steady_clock::now();
    m_holdMods  = 0;

    redraw();
}

void CWindowSwitcher::finish() {
    m_entries.clear();
    m_layouts.clear();
    m_monitor.reset();
    m_selected     = 0;
    m_holdMods     = 0;
    m_closing      = false;
    m_selAnimValid = false;

    redraw();
}

int CWindowSwitcher::tileAt(const Vector2D& pos) const {
    for (const auto& [mon, layout] : m_layouts) {
        for (const auto& [box, idx] : layout.tiles) {
            if (box.containsPoint(pos))
                return sc<int>(idx);
        }
    }

    return -1;
}

bool CWindowSwitcher::onMouseButton(uint32_t button, bool pressed) {
    if (!pressed) {
        const bool SWALLOW = m_swallowRelease;
        m_swallowRelease   = false;
        return SWALLOW;
    }

    if (!active())
        return false;

    m_swallowRelease = true;

    const auto POS = g_pInputManager->getMouseCoordsInternal();
    const int  IDX = tileAt(POS);

    if (IDX >= 0) {
        if (button == 0x110 /* BTN_LEFT */) {
            m_selected = sc<size_t>(IDX);
            commit();
        }
        return true;
    }

    // inside a panel but between tiles: nothing happens; outside all panels: cancel
    for (const auto& [mon, layout] : m_layouts) {
        if (layout.panel.containsPoint(POS))
            return true;
    }

    cancel();
    return true;
}

void CWindowSwitcher::onMouseMove(const Vector2D& pos) {
    if (!active())
        return;

    // the pointer may be resting over a tile when the switcher opens: only real movement selects
    if ((pos - m_openPos).size() < 3.0)
        return;

    const int IDX = tileAt(pos);
    if (IDX >= 0 && sc<size_t>(IDX) != m_selected) {
        m_selected = sc<size_t>(IDX);
        redraw();
    }
}

void CWindowSwitcher::cancel() {
    close();
}

void CWindowSwitcher::commit() {
    if (m_entries.empty())
        return;

    const auto TARGET = m_selected < m_entries.size() ? m_entries[m_selected].window.lock() : nullptr;
    const auto CURRENT = Desktop::focusState()->window();

    close();

    if (!TARGET || TARGET == CURRENT || !validMapped(TARGET))
        return;

    // focusing first switches the workspace and the monitor the window lives on (so a window on another workspace is shown);
    // the switch focuses the workspace's own last window, so the chosen one is focused (and raised) explicitly
    Config::Actions::focus(TARGET);
    Desktop::focusState()->fullWindowFocus(TARGET, Desktop::FOCUS_REASON_DISPATCH_FOCUSWINDOW, nullptr, false);

    // then the plane is moved so that the window lands in the middle of its monitor
    g_layoutManager->centerInfinityOn(TARGET);
}

bool CWindowSwitcher::onKey(xkb_keysym_t sym, bool pressed) {
    if (!active())
        return false;

    // the choice is confirmed by releasing a modifier; the release itself still goes on
    if (!pressed) {
        uint32_t mod = 0;
        switch (sym) {
            case XKB_KEY_Alt_L:
            case XKB_KEY_Alt_R: mod = sc<uint32_t>(Input::HL_MODIFIER_ALT); break;
            case XKB_KEY_Control_L:
            case XKB_KEY_Control_R: mod = sc<uint32_t>(Input::HL_MODIFIER_CTRL); break;
            case XKB_KEY_Super_L:
            case XKB_KEY_Super_R:
            case XKB_KEY_Meta_L:
            case XKB_KEY_Meta_R: mod = sc<uint32_t>(Input::HL_MODIFIER_META); break;
            default: break;
        }

        if (mod & m_holdMods) {
            commit();
            return false;
        }
    }

    switch (sym) {
        case XKB_KEY_Escape:
            if (pressed)
                cancel();
            return true;
        case XKB_KEY_Return:
        case XKB_KEY_KP_Enter:
            if (pressed)
                commit();
            return true;
        case XKB_KEY_Right:
        case XKB_KEY_Down:
            if (pressed)
                step(false);
            return true;
        case XKB_KEY_Left:
        case XKB_KEY_Up:
            if (pressed)
                step(true);
            return true;
        default: break;
    }

    return false;
}

void CWindowSwitcher::updateText(SText& t, const std::string& text, int pt, int maxWidthPx, int weight, const CHyprColor& color) {
    const uint32_t HEX = color.getAsHex();
    if (t.tex && t.pt == pt && t.text == text && t.colorHex == HEX)
        return;

    static auto fontFamily = CConfigValue<std::string>("misc:font_family");

    t.text     = text;
    t.pt       = pt;
    t.colorHex = HEX;
    t.tex      = g_pHyprRenderer->renderText(text, color, pt, false, *fontFamily, maxWidthPx, weight);
    t.size = t.tex ? t.tex->m_size : Vector2D{};
}

void CWindowSwitcher::draw(Render::CRenderContext& ctx, PHLMONITOR monitor) {
    if (m_entries.empty())
        return;

    // windows may have closed while the switcher was open: keep the selection on the same window
    if (std::ranges::any_of(m_entries, [](const SEntry& e) { return !e.window.lock() || !validMapped(e.window.lock()); })) {
        const auto SELECTED = m_selected < m_entries.size() ? m_entries[m_selected].window : PHLWINDOWREF{};

        std::erase_if(m_entries, [](const SEntry& e) { return !e.window.lock() || !validMapped(e.window.lock()); });

        const auto IT       = std::ranges::find_if(m_entries, [&](const SEntry& e) { return e.window.lock() == SELECTED.lock(); });
        m_selected          = IT == m_entries.end() ? std::min(m_selected, m_entries.empty() ? 0 : m_entries.size() - 1) : sc<size_t>(IT - m_entries.begin());
    }

    if (m_entries.size() < 2) {
        finish();
        return;
    }

    // only the monitor the pointer was on when the switcher opened shows the panel
    if (!m_monitor) {
        finish();
        return;
    }
    if (m_monitor != monitor)
        return;

    // the fade-out has run its course
    if (m_closing && std::chrono::steady_clock::now() - m_closeTime >= std::chrono::milliseconds(sc<int>(CLOSE_MS))) {
        finish();
        return;
    }

    const float PROGRESS = m_closing ? animationProgress(m_closeTime, CLOSE_MS, true) : animationProgress(m_openTime, OPEN_MS, false);
    const float EASE     = 1.F - std::pow(1.F - PROGRESS, 3.F);
    const float ALPHA    = m_closing ? PROGRESS : EASE;
    const float SCALEF   = 0.96F + 0.04F * EASE;

    auto& layout = m_layouts[monitor.get()];
    layout       = {};

    // colors come from the one palette (infinity:color_*)
    static auto  PCPANEL = CConfigValue<Config::INTEGER>("infinity:color_panel");
    static auto  PCTILE  = CConfigValue<Config::INTEGER>("infinity:color_tile");
    static auto  PCTILES = CConfigValue<Config::INTEGER>("infinity:color_tile_selected");
    static auto  PCWELL  = CConfigValue<Config::INTEGER>("infinity:color_well");
    static auto  PCACCENT = CConfigValue<Config::INTEGER>("infinity:color_accent");
    static auto  PCTEXT  = CConfigValue<Config::INTEGER>("infinity:color_text");
    static auto  PCSUB   = CConfigValue<Config::INTEGER>("infinity:color_subtext");

    const float  SCALE   = monitor->m_scale;
    const auto   MONSIZE = monitor->m_transformedSize;
    const float  TW      = TILE_W * SCALE;
    const float  TH      = TILE_H * SCALE;
    const float  G       = GAP * SCALE;
    const float  P       = PAD * SCALE;
    const float  INSET   = 12.F * SCALE;
    const float  ICONSZ  = 24.F * SCALE;
    const float  HEADERH = 44.F * SCALE;
    const float  FOOTERH = 32.F * SCALE;
    const int    PT_NAME  = std::clamp(sc<int>(14.F * SCALE), 8, 48);
    const int    PT_TITLE = std::clamp(sc<int>(12.F * SCALE), 8, 48);

    const size_t N    = m_entries.size();
    const size_t COLS = std::clamp<size_t>(sc<size_t>((MONSIZE.x - 2 * P + G) / (TW + G)), 1, N);
    const size_t ROWS = (N + COLS - 1) / COLS;

    const float  PANELW = COLS * TW + (COLS - 1) * G + 2 * P;
    const float  PANELH = ROWS * TH + (ROWS - 1) * G + 2 * P;
    const float  PANELX = std::round((MONSIZE.x - PANELW) / 2.F);
    const float  PANELY = std::round((MONSIZE.y - PANELH) / 2.F);

    // the panel grows a little while it opens and shrinks while it closes; every box goes through this
    const Vector2D CENTER = {PANELX + PANELW / 2.F, PANELY + PANELH / 2.F};
    const auto     SCALEBOX = [&](CBox box) {
        if (SCALEF == 1.F)
            return box;

        box.x = CENTER.x + (box.x - CENTER.x) * SCALEF;
        box.y = CENTER.y + (box.y - CENTER.y) * SCALEF;
        box.w *= SCALEF;
        box.h *= SCALEF;

        return box;
    };
    const auto WITHALPHA = [&](const CHyprColor& color) { return color.modifyA(color.a * ALPHA); };

    // in layout coordinates, for the mouse (the boxes that are really drawn)
    const auto TOLAYOUT = [&](const CBox& box) { return CBox{monitor->m_position.x + box.x / SCALE, monitor->m_position.y + box.y / SCALE, box.w / SCALE, box.h / SCALE}; };
    layout.panel        = TOLAYOUT(SCALEBOX(CBox{PANELX, PANELY, PANELW, PANELH}));

    CRectPassElement::SRectData panel;
    panel.box   = SCALEBOX(CBox{PANELX, PANELY, PANELW, PANELH});
    panel.color = WITHALPHA(CHyprColor(*PCPANEL));
    panel.round = sc<int>(16.F * SCALE * SCALEF);
    ctx.m_pass.add(makeUnique<CRectPassElement>(panel));

    // the selection slides between tiles instead of jumping
    const float TO_COL = sc<float>(m_selected % COLS);
    const float TO_ROW = sc<float>(m_selected / COLS);
    if (!m_selAnimValid) {
        m_selCol       = TO_COL;
        m_selRow       = TO_ROW;
        m_selAnimValid = true;
    }
    const auto  NOW = std::chrono::steady_clock::now();
    const float DT  = std::clamp(std::chrono::duration<float>(NOW - m_animLast).count(), 0.F, 0.1F);
    m_animLast      = NOW;
    const float K   = 1.F - std::exp(-DT * 22.F);
    m_selCol += (TO_COL - m_selCol) * K;
    m_selRow += (TO_ROW - m_selRow) * K;
    if (std::abs(TO_COL - m_selCol) < 0.002F)
        m_selCol = TO_COL;
    if (std::abs(TO_ROW - m_selRow) < 0.002F)
        m_selRow = TO_ROW;

    const float SELX = PANELX + P + m_selCol * (TW + G);
    const float SELY = PANELY + P + m_selRow * (TH + G);

    CRectPassElement::SRectData sel;
    sel.box   = SCALEBOX(CBox{SELX - SELPAD * SCALE, SELY - SELPAD * SCALE, TW + 2 * SELPAD * SCALE, TH + 2 * SELPAD * SCALE});
    sel.color = WITHALPHA(CHyprColor(*PCACCENT));
    sel.round = sc<int>(12.F * SCALE * SCALEF);
    ctx.m_pass.add(makeUnique<CRectPassElement>(sel));

    for (size_t i = 0; i < N; ++i) {
        auto&       e = m_entries[i];
        const auto  W = e.window.lock();

        const float x = PANELX + P + (i % COLS) * (TW + G);
        const float y = PANELY + P + (i / COLS) * (TH + G);

        const CBox TILE = SCALEBOX(CBox{x, y, TW, TH});
        layout.tiles.emplace_back(TOLAYOUT(TILE), i);

        CRectPassElement::SRectData tile;
        tile.box   = TILE;
        tile.color = WITHALPHA(CHyprColor(i == m_selected ? *PCTILES : *PCTILE));
        tile.round = sc<int>(10.F * SCALE * SCALEF);
        ctx.m_pass.add(makeUnique<CRectPassElement>(tile));

        // icon
        if (!e.iconTried) {
            e.iconTried = true;
            if (!e.iconPath.empty()) {
                Hyprgraphics::CImage img(e.iconPath, Vector2D{128, 128});
                if (img.success() && img.cairoSurface())
                    e.icon = g_pHyprRenderer->createTexture(img.cairoSurface()->cairo());
            }
        }

        float textX = x + INSET;
        if (e.icon) {
            CTexPassElement::SRenderData icon;
            icon.tex = e.icon;
            icon.box = SCALEBOX(CBox{x + INSET, y + std::round((HEADERH - ICONSZ) / 2.F), ICONSZ, ICONSZ});
            icon.a   = ALPHA;
            ctx.m_pass.add(makeUnique<CTexPassElement>(std::move(icon)));
            textX += ICONSZ + 8.F * SCALE;
        }

        // application name
        updateText(e.name[PT_NAME], e.appName, PT_NAME, sc<int>(x + TW - INSET - textX), 600, CHyprColor(*PCTEXT));
        auto& name = e.name[PT_NAME];
        if (name.tex) {
            CTexPassElement::SRenderData t;
            t.tex = name.tex;
            t.box = SCALEBOX(CBox{textX, y + std::round((HEADERH - name.size.y) / 2.F), std::min<double>(name.size.x, x + TW - INSET - textX), name.size.y});
            t.a   = ALPHA;
            ctx.m_pass.add(makeUnique<CTexPassElement>(std::move(t)));
        }

        // live preview: the window's own surface texture, fitted into the area under the header
        const CBox AREA = SCALEBOX(CBox{x + INSET, y + HEADERH, TW - 2 * INSET, TH - HEADERH - FOOTERH});

        CRectPassElement::SRectData well;
        well.box   = AREA;
        well.color = WITHALPHA(CHyprColor(*PCWELL));
        well.round = sc<int>(6.F * SCALE);
        ctx.m_pass.add(makeUnique<CRectPassElement>(well));

        const auto SURF    = W->wlSurface() ? W->wlSurface()->resource() : nullptr;
        const auto PREVIEW = SURF ? SURF->m_current.texture : nullptr;
        if (PREVIEW && PREVIEW->m_size.x > 0 && PREVIEW->m_size.y > 0) {
            const double FIT = std::min({AREA.w / PREVIEW->m_size.x, AREA.h / PREVIEW->m_size.y, 1.0 * SCALE});
            const double PW  = PREVIEW->m_size.x * FIT;
            const double PH  = PREVIEW->m_size.y * FIT;

            CTexPassElement::SRenderData pv;
            pv.tex   = PREVIEW;
            pv.box   = {AREA.x + std::round((AREA.w - PW) / 2.0), AREA.y + std::round((AREA.h - PH) / 2.0), PW, PH};
            pv.a     = ALPHA;
            pv.round = sc<int>(4.F * SCALE);
            ctx.m_pass.add(makeUnique<CTexPassElement>(std::move(pv)));
        } else if (e.icon) {
            // nothing to show yet: the big icon stands in for the window
            const float BIG = std::min(AREA.w, AREA.h) * 0.5F;

            CTexPassElement::SRenderData icon;
            icon.tex = e.icon;
            icon.box = {AREA.x + std::round((AREA.w - BIG) / 2.F), AREA.y + std::round((AREA.h - BIG) / 2.F), BIG, BIG};
            icon.a   = 0.8F * ALPHA;
            ctx.m_pass.add(makeUnique<CTexPassElement>(std::move(icon)));
        }

        // window title
        auto& title = e.title[PT_TITLE];
        updateText(title, W->metadata().title(), PT_TITLE, sc<int>(TW - 2 * INSET), 400, CHyprColor(*PCSUB));
        if (title.tex) {
            CTexPassElement::SRenderData t;
            t.tex = title.tex;
            t.box = SCALEBOX(CBox{x + INSET, y + TH - FOOTERH + std::round((FOOTERH - title.size.y) / 2.F), std::min<double>(title.size.x, TW - 2 * INSET), title.size.y});
            t.a   = ALPHA;
            ctx.m_pass.add(makeUnique<CTexPassElement>(std::move(t)));
        }
    }

    // previews are live: keep this part of the screen coming while the switcher is open
    const CBox PANELBOX = layout.panel;
    const CBox DAMAGE   = {PANELBOX.x - 8, PANELBOX.y - 8, PANELBOX.w + 16, PANELBOX.h + 16};
    g_pHyprRenderer->damageBox(DAMAGE);
    monitor->scheduleFrame();
}
