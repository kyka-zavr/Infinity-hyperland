// Copyright (c) 2026, kyka-zavr.

#pragma once

#include "../desktop/DesktopTypes.hpp"
#include "../render/Texture.hpp"
#include "../render/Context.hpp"
#include "../helpers/memory/Memory.hpp"
#include "../helpers/math/Math.hpp"

#include <xkbcommon/xkbcommon.h>
#include <map>
#include <string>
#include <vector>

// The Alt-Tab window switcher (docs/05-infinity-canvas.md): while the modifier that opened it is held, Tab / Shift+Tab / arrows
// move the selection, releasing the modifier focuses the selected window. Windows of all workspaces are listed, most recently used first.
namespace Switcher {
    class CWindowSwitcher {
      public:
        CWindowSwitcher()  = default;
        ~CWindowSwitcher() = default;

        bool active() const;

        // "alttab" / "alttab prev": opens the switcher, or moves the selection when it is already open
        std::string trigger(bool backwards);

        void        commit();
        void        cancel();

        // keyboard events, called before the binds. Returns true when the event was consumed.
        bool        onKey(xkb_keysym_t sym, bool pressed);

        // mouse: a click on a tile picks that window, a click outside the panels cancels. Both return true when consumed.
        bool        onMouseButton(uint32_t button, bool pressed);
        void        onMouseMove(const Vector2D& pos); // hovering a tile selects it

        // drawn on every monitor
        void        draw(Render::CRenderContext& ctx, PHLMONITOR monitor);

      private:
        struct SText {
            SP<Render::ITexture> tex;
            Vector2D             size;
            std::string          text;
            int                  pt       = 0;
            uint32_t             colorHex = 0; // a palette change re-renders the text
        };

        struct SEntry {
            PHLWINDOWREF         window;
            std::string          appName, iconPath;
            SP<Render::ITexture> icon;
            bool                 iconTried = false;
            std::map<int, SText> name, title; // by font size: monitors may have different scales
        };

        void                step(bool backwards);
        void                updateText(SText& t, const std::string& text, int pt, int maxWidthPx, int weight, const CHyprColor& color);
        void                close();
        void                redraw();

        std::vector<SEntry> m_entries;
        size_t              m_selected = 0;
        uint32_t            m_holdMods = 0; // modifiers whose release confirms the choice

        // where the panel and its tiles are on each monitor (layout coordinates), filled while drawing
        struct SLayout {
            CBox                                  panel;
            std::vector<std::pair<CBox, size_t>> tiles;
        };
        std::map<const void*, SLayout>    m_layouts;
        Vector2D                           m_openPos;
        bool                               m_swallowRelease = false;

        int                                tileAt(const Vector2D& pos) const;
    };

    UP<CWindowSwitcher>& switcher();
}
