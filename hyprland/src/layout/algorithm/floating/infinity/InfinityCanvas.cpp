// Copyright (c) 2026, kyka-zavr.

#include "InfinityCanvas.hpp"

#include "../../Algorithm.hpp"
#include "../../../space/Space.hpp"

#include "../../../../desktop/view/window/Window.hpp"
#include "../../../../event/EventBus.hpp"
#include "../../../../state/WorkspaceState.hpp"
#include "../../../../workspace/HLWorkspace.hpp"

using namespace Layout;
using namespace Layout::Floating;

UP<CInfinityCanvas>& Floating::canvas() {
    static UP<CInfinityCanvas> c = makeUnique<CInfinityCanvas>();
    return c;
}

CInfinityCanvas::CInfinityCanvas() {
    // a workspace that is switched to while the mode is on joins the mode at once
    m_wsActive  = Event::bus()->m_events.workspace.active.listen([this](PHLWORKSPACE workspace) { applyTo(workspace); });
    m_wsCreated = Event::bus()->m_events.workspace.createdLate.listen([this](PHLWORKSPACEREF workspace) { applyTo(workspace.lock()); });
}

bool Floating::isCanvasWindow(PHLWINDOW window) {
    if (!window || !window->m_workspace || !window->m_workspace->space())
        return false;

    const auto ALGORITHM = window->m_workspace->space()->algorithm();
    return ALGORITHM && ALGORITHM->infinityActive();
}

bool CInfinityCanvas::enabled() const {
    return m_enabled;
}

void CInfinityCanvas::setEnabled(bool enabled) {
    if (m_enabled == enabled)
        return;

    m_enabled = enabled;
    applyToAll();
}

void CInfinityCanvas::toggle() {
    setEnabled(!m_enabled);
}

void CInfinityCanvas::applyTo(PHLWORKSPACE workspace) {
    if (!workspace || !workspace->space())
        return;

    // special workspaces (scratchpads) are drawn as an overlay and keep their own behaviour
    if (workspace->type() == Workspace::eWorkspaceType::SPECIAL)
        return;

    const auto ALGORITHM = workspace->space()->algorithm();
    if (!ALGORITHM)
        return;

    ALGORITHM->setInfinity(m_enabled);
}

void CInfinityCanvas::applyToAll() {
    for (const auto& workspace : State::Workspace::state()->workspaces())
        applyTo(workspace.lock());
}
