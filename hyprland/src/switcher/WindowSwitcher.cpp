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
#include <ranges>

using namespace Switcher;

// logical pixels, multiplied by the monitor scale when drawn
static constexpr float TILE_W = 280.F;
static constexpr float TILE_H = 226.F;
static constexpr float GAP    = 12.F;
static constexpr float PAD    = 20.F;
static constexpr float SELPAD = 3.F;

UP<CWindowSwitcher>& Switcher::switcher() {
    static UP<CWindowSwitcher> p = makeUnique<CWindowSwitcher>();
    return p;
}

bool CWindowSwitcher::active() const {
    return !m_entries.empty();
}

std::string CWindowSwitcher::trigger(bool backwards) {
    if (active()) {
        step(backwards);
        return {};
    }

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

void CWindowSwitcher::close() {
    m_entries.clear();
    m_layouts.clear();
    m_selected = 0;
    m_holdMods = 0;

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

    // in Infinity the plane is first moved so the window lands in the middle of its monitor, otherwise the workspace is switched as for focuswindow
    g_layoutManager->centerInfinityOn(TARGET);
    Config::Actions::focus(TARGET);
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
    if (t.tex && t.pt == pt && t.text == text)
        return;

    static auto fontFamily = CConfigValue<std::string>("misc:font_family");

    t.text = text;
    t.pt   = pt;
    t.tex  = g_pHyprRenderer->renderText(text, color, pt, false, *fontFamily, maxWidthPx, weight);
    t.size = t.tex ? t.tex->m_size : Vector2D{};
}

void CWindowSwitcher::draw(Render::CRenderContext& ctx, PHLMONITOR monitor) {
    if (!active())
        return;

    // windows may have closed while the switcher was open
    std::erase_if(m_entries, [](const SEntry& e) { return !e.window.lock() || !validMapped(e.window.lock()); });
    if (m_entries.size() < 2) {
        close();
        return;
    }
    m_selected = std::min(m_selected, m_entries.size() - 1);

    auto& layout = m_layouts[monitor.get()];
    layout       = {};

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

    // in layout coordinates, for the mouse
    const auto TOLAYOUT = [&](float px, float py, float pw, float ph) { return CBox{monitor->m_position.x + px / SCALE, monitor->m_position.y + py / SCALE, pw / SCALE, ph / SCALE}; };
    layout.panel        = TOLAYOUT(PANELX, PANELY, PANELW, PANELH);

    CRectPassElement::SRectData panel;
    panel.box   = {PANELX, PANELY, PANELW, PANELH};
    panel.color = CHyprColor{0.07F, 0.07F, 0.09F, 0.94F};
    panel.round = sc<int>(16.F * SCALE);
    ctx.m_pass.add(makeUnique<CRectPassElement>(panel));

    for (size_t i = 0; i < N; ++i) {
        auto&       e = m_entries[i];
        const auto  W = e.window.lock();

        const float x = PANELX + P + (i % COLS) * (TW + G);
        const float y = PANELY + P + (i / COLS) * (TH + G);

        layout.tiles.emplace_back(TOLAYOUT(x, y, TW, TH), i);

        if (i == m_selected) {
            CRectPassElement::SRectData sel;
            sel.box   = {x - SELPAD * SCALE, y - SELPAD * SCALE, TW + 2 * SELPAD * SCALE, TH + 2 * SELPAD * SCALE};
            sel.color = CHyprColor{0.36F, 0.56F, 1.F, 1.F};
            sel.round = sc<int>(12.F * SCALE);
            ctx.m_pass.add(makeUnique<CRectPassElement>(sel));
        }

        CRectPassElement::SRectData tile;
        tile.box   = {x, y, TW, TH};
        tile.color = i == m_selected ? CHyprColor{0.16F, 0.17F, 0.22F, 1.F} : CHyprColor{0.12F, 0.12F, 0.15F, 1.F};
        tile.round = sc<int>(10.F * SCALE);
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
            icon.box = {x + INSET, y + std::round((HEADERH - ICONSZ) / 2.F), ICONSZ, ICONSZ};
            icon.a   = 1.F;
            ctx.m_pass.add(makeUnique<CTexPassElement>(std::move(icon)));
            textX += ICONSZ + 8.F * SCALE;
        }

        // application name
        updateText(e.name[PT_NAME], e.appName, PT_NAME, sc<int>(x + TW - INSET - textX), 600, CHyprColor{1.F, 1.F, 1.F, 1.F});
        auto& name = e.name[PT_NAME];
        if (name.tex) {
            CTexPassElement::SRenderData t;
            t.tex = name.tex;
            t.box = {textX, y + std::round((HEADERH - name.size.y) / 2.F), std::min<double>(name.size.x, x + TW - INSET - textX), name.size.y};
            t.a   = 1.F;
            ctx.m_pass.add(makeUnique<CTexPassElement>(std::move(t)));
        }

        // live preview: the window's own surface texture, fitted into the area under the header
        const CBox AREA = {x + INSET, y + HEADERH, TW - 2 * INSET, TH - HEADERH - FOOTERH};

        CRectPassElement::SRectData well;
        well.box   = AREA;
        well.color = CHyprColor{0.05F, 0.05F, 0.07F, 1.F};
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
            pv.a     = 1.F;
            pv.round = sc<int>(4.F * SCALE);
            ctx.m_pass.add(makeUnique<CTexPassElement>(std::move(pv)));
        } else if (e.icon) {
            // nothing to show yet: the big icon stands in for the window
            const float BIG = std::min(AREA.w, AREA.h) * 0.5F;

            CTexPassElement::SRenderData icon;
            icon.tex = e.icon;
            icon.box = {AREA.x + std::round((AREA.w - BIG) / 2.F), AREA.y + std::round((AREA.h - BIG) / 2.F), BIG, BIG};
            icon.a   = 0.8F;
            ctx.m_pass.add(makeUnique<CTexPassElement>(std::move(icon)));
        }

        // window title
        auto& title = e.title[PT_TITLE];
        updateText(title, W->metadata().title(), PT_TITLE, sc<int>(TW - 2 * INSET), 400, CHyprColor{0.72F, 0.74F, 0.8F, 1.F});
        if (title.tex) {
            CTexPassElement::SRenderData t;
            t.tex = title.tex;
            t.box = {x + INSET, y + TH - FOOTERH + std::round((FOOTERH - title.size.y) / 2.F), std::min<double>(title.size.x, TW - 2 * INSET), title.size.y};
            t.a   = 1.F;
            ctx.m_pass.add(makeUnique<CTexPassElement>(std::move(t)));
        }
    }

    // previews are live: keep this part of the screen coming while the switcher is open
    const CBox DAMAGE = {monitor->m_position.x + PANELX / SCALE - 4, monitor->m_position.y + PANELY / SCALE - 4, PANELW / SCALE + 8, PANELH / SCALE + 8};
    g_pHyprRenderer->damageBox(DAMAGE);
    monitor->scheduleFrame();
}
