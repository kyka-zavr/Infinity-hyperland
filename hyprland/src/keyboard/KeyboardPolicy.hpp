// Copyright (c) 2026, kyka-zavr.

#pragma once

#include "../desktop/DesktopTypes.hpp"
#include "../helpers/memory/Memory.hpp"

#include <string>
#include <vector>

class IKeyboard;

// Keyboard layout and Num Lock policy (docs: usage.md, "Keyboard layouts").
//
//   infinity:kb_layout_mode   system | per_window    one layout for the session, or every window remembers its own
//   infinity:kb_default_layout  -1 | 0..N              layout a NEW window starts with in per_window mode (-1: keep the current one)
//   infinity:num_mode        normal | per_window | always
//                              per_window: Num Lock is ON in new windows and remembered per window; always: Num Lock is always on
//
// The layouts themselves are the usual input:kb_layout list. All of this sits on top of IKeyboard::updateModifiers.
namespace KeyboardPolicy {
    class CPolicy {
      public:
        CPolicy();
        ~CPolicy() = default;

        // keyboard state changed (key press, layout switch, Num Lock...)
        void onModifiers(const SP<IKeyboard>& keyboard);

        // the keymap was rebuilt (layouts changed, config reload): state is reapplied
        void onKeymap(const SP<IKeyboard>& keyboard);

        // "next", "prev" or a layout index, for all keyboards. Returns an error text, empty when fine.
        std::string switchLayout(const std::string& arg);

      private:
        struct SWindowState {
            PHLWINDOWREF window;
            uint32_t     group = 0;
            bool         num  = false;
        };

        enum class eMode : uint8_t {
            SYSTEM,
            PER_WINDOW,
            ALWAYS,
        };

        eMode         layoutMode() const;
        eMode         numMode() const;

        void          onFocus(PHLWINDOW window);
        SWindowState& stateFor(PHLWINDOW window);
        void          apply(uint32_t group, bool num, bool applyGroup, bool applyNum);
        void          prune();

        std::vector<SWindowState> m_states;
        PHLWINDOWREF              m_current;
        bool                      m_applying = false;
    };

    UP<CPolicy>& policy();
}
