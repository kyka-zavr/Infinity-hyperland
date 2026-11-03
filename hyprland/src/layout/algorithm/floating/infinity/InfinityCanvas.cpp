// Copyright (c) 2026, kyka-zavr.

#include "InfinityCanvas.hpp"

#include "../../Algorithm.hpp"
#include "../../../space/Space.hpp"

#include "../../../../desktop/view/window/Window.hpp"
#include "../../../../event/EventBus.hpp"
#include "../../../../output/Monitor.hpp"
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

bool Floating::isCanvasWindow(PHLWINDOWREF window) {
    return isCanvasWindow(window.lock());
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

Vector2D CInfinityCanvas::view() const {
    return m_view;
}

float CInfinityCanvas::zoom() const {
    return m_zoom;
}

void CInfinityCanvas::setView(const Vector2D& view) {
    m_view = view;
}

void CInfinityCanvas::setZoom(float zoom) {
    m_zoom = zoom;
}

void CInfinityCanvas::pan(const Vector2D& Δ) {
    m_view -= Δ;
}

bool CInfinityCanvas::zoomBy(float mult, const Vector2D& anchor) {
    constexpr float MIN_ZOOM = 0.1F;
    constexpr float MAX_ZOOM = 4.F;

    const float     NEW_ZOOM = std::clamp(m_zoom * mult, MIN_ZOOM, MAX_ZOOM);
    if (NEW_ZOOM == m_zoom)
        return false;

    // the point under the anchor stays under it: view' = view + anchor / zoom - anchor / zoom'
    m_view += anchor / m_zoom - anchor / NEW_ZOOM;
    m_zoom = NEW_ZOOM;

    return true;
}

CBox CInfinityCanvas::visibleRect(PHLMONITOR monitor) const {
    if (!monitor)
        return {};

    if (m_zoom == 1.F)
        return {monitor->m_position, monitor->m_size};

    return {monitor->m_position / m_zoom, monitor->m_size / m_zoom};
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
