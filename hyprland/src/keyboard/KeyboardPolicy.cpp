// Copyright (c) 2026, kyka-zavr.

#include "KeyboardPolicy.hpp"

#include "../config/ConfigValue.hpp"
#include "../desktop/view/window/Window.hpp"
#include "../devices/IKeyboard.hpp"
#include "../event/EventBus.hpp"
#include "../managers/input/InputManager.hpp"

#include <algorithm>
#include <xkbcommon/xkbcommon.h>

using namespace KeyboardPolicy;

UP<CPolicy>& KeyboardPolicy::policy() {
    static UP<CPolicy> p = makeUnique<CPolicy>();
    return p;
}

CPolicy::CPolicy() {
    static auto P = Event::bus()->m_events.window.active.listen([this](PHLWINDOW window, Desktop::eFocusReason) { onFocus(window); });
}

CPolicy::eMode CPolicy::layoutMode() const {
    static auto PMODE = CConfigValue<Config::STRING>("infinity:kb_layout_mode");
    return std::string_view{*PMODE} == "per_window" ? eMode::PER_WINDOW : eMode::SYSTEM;
}

CPolicy::eMode CPolicy::capsMode() const {
    static auto PMODE = CConfigValue<Config::STRING>("infinity:caps_mode");
    const auto  MODE  = std::string_view{*PMODE};
    return MODE == "per_window" ? eMode::PER_WINDOW : (MODE == "always" ? eMode::ALWAYS : eMode::SYSTEM);
}

// the state is read from xkb itself: the copy in m_modifiersState can lag behind right after a keymap rebuild
static uint32_t capsBit(const SP<IKeyboard>& kb) {
    if (!kb || !kb->m_xkbKeymap)
        return 0;

    const auto IDX = xkb_keymap_mod_get_index(kb->m_xkbKeymap, XKB_MOD_NAME_CAPS);
    return IDX == XKB_MOD_INVALID ? 0 : (sc<uint32_t>(1) << IDX);
}

static uint32_t layoutCount(const SP<IKeyboard>& kb) {
    return kb && kb->m_xkbKeymap ? std::max<uint32_t>(1, xkb_keymap_num_layouts(kb->m_xkbKeymap)) : 1;
}

static uint32_t lockedNow(const SP<IKeyboard>& kb) {
    return kb && kb->m_xkbState ? xkb_state_serialize_mods(kb->m_xkbState, XKB_STATE_MODS_LOCKED) : 0;
}

static uint32_t groupNow(const SP<IKeyboard>& kb) {
    return kb && kb->m_xkbState ? xkb_state_serialize_layout(kb->m_xkbState, XKB_STATE_LAYOUT_EFFECTIVE) : 0;
}

void CPolicy::prune() {
    std::erase_if(m_states, [](const SWindowState& s) { return s.window.expired(); });
}

CPolicy::SWindowState& CPolicy::stateFor(PHLWINDOW window) {
    for (auto& s : m_states) {
        if (s.window.lock() == window)
            return s;
    }

    // a window seen for the first time starts from the configured defaults
    static auto PDEFAULT = CConfigValue<Config::INTEGER>("infinity:kb_default_layout");

    SWindowState state;
    state.window = window;

    const auto FIRST = g_pInputManager->m_keyboards.empty() ? nullptr : g_pInputManager->m_keyboards.front();

    state.group = *PDEFAULT >= 0 ? std::min<uint32_t>(*PDEFAULT, layoutCount(FIRST) - 1) : groupNow(FIRST);
    state.caps  = capsMode() == eMode::PER_WINDOW ? true : (lockedNow(FIRST) & capsBit(FIRST)) != 0;

    m_states.emplace_back(state);
    return m_states.back();
}

void CPolicy::apply(uint32_t group, bool caps, bool applyGroup, bool applyCaps) {
    m_applying = true;

    for (const auto& kb : g_pInputManager->m_keyboards) {
        if (!kb || !kb->m_xkbState)
            continue;

        const auto BIT        = capsBit(kb);
        const auto LOCKED_NOW = lockedNow(kb);
        const auto GROUP_NOW  = groupNow(kb);

        auto       locked = LOCKED_NOW;
        auto       grp    = GROUP_NOW;

        if (applyCaps && BIT)
            locked = caps ? (locked | BIT) : (locked & ~BIT);
        if (applyGroup)
            grp = std::min<uint32_t>(group, layoutCount(kb) - 1);

        if (locked == LOCKED_NOW && grp == GROUP_NOW)
            continue;

        kb->updateModifiers(xkb_state_serialize_mods(kb->m_xkbState, XKB_STATE_MODS_DEPRESSED), xkb_state_serialize_mods(kb->m_xkbState, XKB_STATE_MODS_LATCHED), locked, grp);
    }

    m_applying = false;
}

void CPolicy::onFocus(PHLWINDOW window) {
    m_current = window;
    if (!window)
        return;

    const auto LM = layoutMode();
    const auto CM = capsMode();
    if (LM != eMode::PER_WINDOW && CM != eMode::PER_WINDOW)
        return;

    prune();

    const auto STATE = stateFor(window); // a copy: apply() never touches the list, but be safe about reallocations
    apply(STATE.group, STATE.caps, LM == eMode::PER_WINDOW, CM == eMode::PER_WINDOW);
}

void CPolicy::onModifiers(const SP<IKeyboard>& keyboard) {
    if (m_applying || !keyboard)
        return;

    const auto CM  = capsMode();
    const auto BIT = capsBit(keyboard);

    if (CM == eMode::ALWAYS) {
        if (BIT && !(lockedNow(keyboard) & BIT))
            apply(0, true, false, true); // Caps Lock was switched off: put it back
        return;
    }

    const auto LM = layoutMode();
    if (LM != eMode::PER_WINDOW && CM != eMode::PER_WINDOW)
        return;

    const auto CURRENT = m_current.lock();
    if (!CURRENT)
        return;

    auto& state = stateFor(CURRENT);
    if (LM == eMode::PER_WINDOW)
        state.group = groupNow(keyboard);
    if (CM == eMode::PER_WINDOW && BIT)
        state.caps = (lockedNow(keyboard) & BIT) != 0;
}

void CPolicy::onKeymap(const SP<IKeyboard>& keyboard) {
    if (m_applying || !keyboard)
        return;

    // a rebuilt keymap starts with clean locks and the first layout
    if (capsMode() == eMode::ALWAYS)
        apply(0, true, false, true);
    else if (const auto CURRENT = m_current.lock(); CURRENT)
        onFocus(CURRENT);
}

std::string CPolicy::switchLayout(const std::string& arg) {
    if (g_pInputManager->m_keyboards.empty())
        return "no keyboard";

    const auto FIRST = g_pInputManager->m_keyboards.front();
    const auto COUNT = layoutCount(FIRST);
    const auto NOW   = groupNow(FIRST);

    uint32_t   target = NOW;
    if (arg == "next")
        target = (NOW + 1) % COUNT;
    else if (arg == "prev")
        target = (NOW + COUNT - 1) % COUNT;
    else {
        try {
            const int IDX = std::stoi(arg);
            if (IDX < 0 || sc<uint32_t>(IDX) >= COUNT)
                return std::format("layout index out of range (0 to {})", COUNT - 1);
            target = IDX;
        } catch (...) { return "expected next, prev or a layout index"; }
    }

    apply(target, false, true, false);

    // apply() silences onModifiers, so the choice is remembered here
    if (layoutMode() == eMode::PER_WINDOW) {
        if (const auto CURRENT = m_current.lock(); CURRENT)
            stateFor(CURRENT).group = target;
    }

    return {};
}
