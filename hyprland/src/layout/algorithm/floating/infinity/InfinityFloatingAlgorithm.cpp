// Copyright (c) 2026, kyka-zavr.

#include "InfinityFloatingAlgorithm.hpp"

#include "../../Algorithm.hpp"

#include "../../../target/Target.hpp"
#include "../../../../desktop/view/window/Window.hpp"
#include "../../../../output/Monitor.hpp"
#include "../../../../render/Renderer.hpp"

#include <algorithm>

using namespace Layout;
using namespace Layout::Floating;

Vector2D                                         CInfinityFloatingAlgorithm::s_view      = {};
float                                            CInfinityFloatingAlgorithm::s_zoom      = 1.F;
std::vector<CInfinityFloatingAlgorithm*>         CInfinityFloatingAlgorithm::s_instances = {};

CInfinityFloatingAlgorithm::CInfinityFloatingAlgorithm() : m_applied(s_view) {
    s_instances.emplace_back(this);
}

CInfinityFloatingAlgorithm::~CInfinityFloatingAlgorithm() {
    std::erase(s_instances, this);

    // the last Infinity workspace is gone: the next one starts from a clean plane
    if (s_instances.empty()) {
        s_view = {};
        s_zoom = 1.F;
    }
}

void CInfinityFloatingAlgorithm::damageMonitor() const {
    if (const auto PARENT = m_parent.lock(); PARENT && PARENT->space() && PARENT->space()->workspace())
        g_pHyprRenderer->damageMonitor(PARENT->space()->workspace()->m_monitor.lock());
}

void CInfinityFloatingAlgorithm::newTarget(SP<ITarget> target) {
    // a window that is already mapped and has a place (mode switch) keeps it
    const auto WINDOW = target->window();
    if (WINDOW && !(WINDOW->m_state & Desktop::View::WINDOW_STATE_FIRST_MAP) && target->position().w > 1 && target->position().h > 1) {
        remember(target);
        return;
    }

    CDefaultFloatingAlgorithm::newTarget(target);
    remember(target);
}

void CInfinityFloatingAlgorithm::movedTarget(SP<ITarget> target, std::optional<Vector2D> focalPoint) {
    CDefaultFloatingAlgorithm::movedTarget(target, focalPoint);
    remember(target);
}

void CInfinityFloatingAlgorithm::removeTarget(SP<ITarget> target) {
    CDefaultFloatingAlgorithm::removeTarget(target);
    m_world.erase(target);
    m_floatingBefore.erase(target);
}

void CInfinityFloatingAlgorithm::resizeTarget(const Vector2D& Δ, SP<ITarget> target, eRectCorner corner) {
    CDefaultFloatingAlgorithm::resizeTarget(Δ, target, corner);
    remember(target);
}

void CInfinityFloatingAlgorithm::moveTarget(const Vector2D& Δ, SP<ITarget> target) {
    CDefaultFloatingAlgorithm::moveTarget(Δ, target);
    remember(target);
}

void CInfinityFloatingAlgorithm::setTargetGeom(const CBox& geom, SP<ITarget> target) {
    CDefaultFloatingAlgorithm::setTargetGeom(geom, target);
    remember(target);
}

void CInfinityFloatingAlgorithm::swapTargets(SP<ITarget> a, SP<ITarget> b) {
    CDefaultFloatingAlgorithm::swapTargets(a, b);
    remember(a);
    remember(b);
}

void CInfinityFloatingAlgorithm::moveTargetInDirection(SP<ITarget> t, Math::eDirection dir, bool silent) {
    CDefaultFloatingAlgorithm::moveTargetInDirection(t, dir, silent);
    remember(t);
}

void CInfinityFloatingAlgorithm::recenter(SP<ITarget> t) {
    CDefaultFloatingAlgorithm::recenter(t);
    remember(t);
}

void CInfinityFloatingAlgorithm::adopt(SP<ITarget> target) {
    remember(target);
}

void CInfinityFloatingAlgorithm::pan(const Vector2D& Δ) {
    s_view -= Δ;

    // copy: placing targets can end up in code that creates or destroys algorithms
    const auto ALL = s_instances;
    for (const auto& a : ALL) {
        a->applyView();
    }
}

void CInfinityFloatingAlgorithm::zoomBy(float mult, const Vector2D& anchor) {
    constexpr float MIN_ZOOM = 0.1F;
    constexpr float MAX_ZOOM = 4.F;

    const float     NEW_ZOOM = std::clamp(s_zoom * mult, MIN_ZOOM, MAX_ZOOM);
    if (NEW_ZOOM == s_zoom)
        return;

    // the world point under the anchor stays under it: view' = view + anchor / zoom - anchor / zoom'
    s_view += anchor / s_zoom - anchor / NEW_ZOOM;
    s_zoom = NEW_ZOOM;

    const auto ALL = s_instances;
    for (const auto& a : ALL) {
        a->applyView();
        a->damageMonitor();
    }
}

void CInfinityFloatingAlgorithm::resetView() {
    // only our own targets go back to their world positions; the shared view stays for the other workspaces
    const auto SHARED = s_view;
    s_view            = {};
    applyView();
    s_view = SHARED;

    damageMonitor();
}

float CInfinityFloatingAlgorithm::zoom() const {
    return s_zoom;
}

void CInfinityFloatingAlgorithm::rememberFloatingBefore(SP<ITarget> target) {
    m_floatingBefore.emplace(target);
}

bool CInfinityFloatingAlgorithm::wasFloatingBefore(SP<ITarget> target) const {
    return m_floatingBefore.contains(target);
}

Vector2D CInfinityFloatingAlgorithm::view() const {
    return s_view;
}

void CInfinityFloatingAlgorithm::remember(SP<ITarget> target) {
    auto box = target->position();
    box.translate(m_applied);
    m_world[target] = box;
}

void CInfinityFloatingAlgorithm::applyView() {
    m_applied = s_view;

    std::erase_if(m_world, [](const auto& e) { return e.first.expired(); });

    for (const auto& [wp, world] : m_world) {
        const auto TARGET = wp.lock();
        if (!TARGET)
            continue;

        auto box = world;
        box.translate(-s_view);

        TARGET->damageEntire();
        TARGET->setPositionGlobal(box);
        TARGET->warpPositionSize();
        TARGET->damageEntire();
    }
}
