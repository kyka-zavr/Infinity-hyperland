// Copyright (c) 2026, kyka-zavr.

#include "InfinityFloatingAlgorithm.hpp"

#include "../../Algorithm.hpp"

#include "../../../target/Target.hpp"
#include "../../../../config/ConfigValue.hpp"
#include "../../../../config/shared/actions/ConfigActions.hpp"
#include "../../../../desktop/view/window/Window.hpp"
#include "../../../../desktop/rule/windowRule/WindowRuleApplicator.hpp"
#include "../../../../output/Monitor.hpp"
#include "../../../../render/Renderer.hpp"

#include <algorithm>
#include <ranges>

using namespace Layout;
using namespace Layout::Floating;

bool Layout::Floating::isOverlayWindow(PHLWINDOW window) {
    static auto                     PCLASSES = CConfigValue<Config::STRING>("infinity:overlay_classes");
    static std::string              LAST;
    static std::vector<std::string> CLASSES;

    if (LAST != *PCLASSES) {
        LAST = *PCLASSES;
        CLASSES.clear();

        std::string current;
        for (const char C : LAST) {
            if (C == ' ' || C == ',' || C == '\t') {
                if (!current.empty()) {
                    CLASSES.emplace_back(current);
                    current.clear();
                }

                continue;
            }

            current += C;
        }

        if (!current.empty())
            CLASSES.emplace_back(current);
    }

    if (!window || CLASSES.empty())
        return false;

    const auto APPID = window->metadata().appID().empty() ? window->metadata().initialAppID() : window->metadata().appID();
    return std::ranges::contains(CLASSES, APPID);
}

// last canvas box of every window, shared by all instances: leaving Infinity tiling changes the windows, entering it again
// puts them back where they were on the plane
static std::map<WP<ITarget>, CBox>& savedCanvasBoxes() {
    static std::map<WP<ITarget>, CBox> saved;
    return saved;
}

// the view the saved boxes were stored with, so entering the mode again returns to the same part of the plane
static Vector2D s_savedView     = {};
static float    s_savedZoom     = 1.F;
static bool     s_haveSavedView = false;

Vector2D                                         CInfinityFloatingAlgorithm::s_view      = {};
float                                            CInfinityFloatingAlgorithm::s_zoom      = 1.F;
std::vector<CInfinityFloatingAlgorithm*>         CInfinityFloatingAlgorithm::s_instances = {};

CInfinityFloatingAlgorithm::CInfinityFloatingAlgorithm() : m_applied(s_view) {
    s_instances.emplace_back(this);
}

CInfinityFloatingAlgorithm::~CInfinityFloatingAlgorithm() {
    std::erase(s_instances, this);

    // the last Infinity workspace is gone: a workspace without a saved canvas starts from a clean plane
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
    // an overlay window (the settings window) is not part of the plane: it is pinned to the monitor
    if (arrangeOverlay(target))
        return;

    // a window that was in the mode before the last exit returns to the place and size it had on the canvas
    if (restoreCanvasBox(target)) {
        remember(target);
        return;
    }

    // a window that is already mapped and has a place (mode switch) keeps it
    const auto WINDOW = target->window();
    if (WINDOW && !(WINDOW->m_state & Desktop::View::WINDOW_STATE_FIRST_MAP) && target->position().w > 1 && target->position().h > 1) {
        remember(target);
        return;
    }

    CDefaultFloatingAlgorithm::newTarget(target);
    centerNewTarget(target);
    remember(target);
}

void CInfinityFloatingAlgorithm::movedTarget(SP<ITarget> target, std::optional<Vector2D> focalPoint) {
    CDefaultFloatingAlgorithm::movedTarget(target, focalPoint);

    if (!arrangeOverlay(target))
        remember(target);
}

void CInfinityFloatingAlgorithm::removeTarget(SP<ITarget> target) {
    CDefaultFloatingAlgorithm::removeTarget(target);
    m_world.erase(target);
    m_floatingBefore.erase(target);
}

void CInfinityFloatingAlgorithm::resizeTarget(const Vector2D& Δ, SP<ITarget> target, eRectCorner corner) {
    CDefaultFloatingAlgorithm::resizeTarget(Δ, target, corner);

    if (!arrangeOverlay(target))
        remember(target);
}

void CInfinityFloatingAlgorithm::moveTarget(const Vector2D& Δ, SP<ITarget> target) {
    CDefaultFloatingAlgorithm::moveTarget(Δ, target);

    if (!arrangeOverlay(target))
        remember(target);
}

void CInfinityFloatingAlgorithm::setTargetGeom(const CBox& geom, SP<ITarget> target) {
    CDefaultFloatingAlgorithm::setTargetGeom(geom, target);

    if (!arrangeOverlay(target))
        remember(target);
}

void CInfinityFloatingAlgorithm::swapTargets(SP<ITarget> a, SP<ITarget> b) {
    CDefaultFloatingAlgorithm::swapTargets(a, b);

    if (!arrangeOverlay(a))
        remember(a);
    if (!arrangeOverlay(b))
        remember(b);
}

void CInfinityFloatingAlgorithm::moveTargetInDirection(SP<ITarget> t, Math::eDirection dir, bool silent) {
    CDefaultFloatingAlgorithm::moveTargetInDirection(t, dir, silent);

    if (!arrangeOverlay(t))
        remember(t);
}

void CInfinityFloatingAlgorithm::recenter(SP<ITarget> t) {
    CDefaultFloatingAlgorithm::recenter(t);

    if (!arrangeOverlay(t))
        remember(t);
}

void CInfinityFloatingAlgorithm::adopt(SP<ITarget> target) {
    if (!arrangeOverlay(target))
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

        TARGET->damageEntire();

        // overlay windows are drawn unscaled on top of the plane: they stay in the middle of their monitor
        if (isOverlayWindow(TARGET->window())) {
            centerOverlay(TARGET);
            TARGET->damageEntire();
            continue;
        }

        auto box = world;
        box.translate(-s_view);

        TARGET->setPositionGlobal(box);
        TARGET->warpPositionSize();
        TARGET->damageEntire();
    }
}

void CInfinityFloatingAlgorithm::centerNewTarget(SP<ITarget> target) {
    if (!target || s_zoom == 1.F)
        return;

    // the default placement is in layout coordinates, but the plane is drawn scaled (screen = layout * zoom): only a scaled
    // position ends up in the middle of the monitor
    const auto WINDOW = target->window();
    if (WINDOW && (WINDOW->m_ruleApplicator->static_.position || WINDOW->m_ruleApplicator->static_.center.value_or(false)))
        return;

    const auto MONITOR = m_parent && m_parent->space() && m_parent->space()->workspace() ? m_parent->space()->workspace()->m_monitor.lock() : nullptr;
    if (!MONITOR)
        return;

    const auto SIZE = target->position().size();
    target->setPositionGlobal(CBox{(MONITOR->m_position + MONITOR->m_size / 2.F) / s_zoom - SIZE / 2.F, SIZE});
    target->warpPositionSize();
}

void CInfinityFloatingAlgorithm::storeCanvasBoxes() const {
    auto& SAVED = savedCanvasBoxes();
    std::erase_if(SAVED, [](const auto& e) { return e.first.expired(); });

    for (const auto& [target, box] : m_world) {
        if (!target.expired())
            SAVED[target] = box;
    }

    if (!SAVED.empty()) {
        s_savedView     = s_view;
        s_savedZoom     = s_zoom;
        s_haveSavedView = true;
    }
}

// entering the mode: if some of these windows were on the canvas before the last exit, the plane returns to the view they
// were left at; otherwise the workspace starts from a clean plane
bool CInfinityFloatingAlgorithm::prepareCanvas(const std::vector<SP<ITarget>>& targets) {
    const auto& SAVED = savedCanvasBoxes();
    // overlay windows are always in the middle of the monitor: they do not make a workspace "the one that was on the canvas"
    const bool ANY = std::ranges::any_of(targets, [&](const SP<ITarget>& target) { return target && !isOverlayWindow(target->window()) && SAVED.contains(target); });

    if (!ANY) {
        // a workspace that was never on the canvas starts from a clean plane, unless another workspace is still in the mode
        if (s_instances.empty()) {
            s_view = {};
            s_zoom = 1.F;
        }

        return false;
    }

    // another Infinity workspace is still in the mode: its shared view must not be disturbed
    if (s_instances.empty() && s_haveSavedView) {
        s_view = s_savedView;
        s_zoom = s_savedZoom;
    }

    return true;
}

bool CInfinityFloatingAlgorithm::restoreCanvasBox(SP<ITarget> target) {
    auto&      SAVED = savedCanvasBoxes();
    const auto IT    = SAVED.find(target);
    if (IT == SAVED.end())
        return false;

    auto box = IT->second;
    SAVED.erase(IT);

    // the box is a world box, the view may have changed since it was stored
    box.translate(-s_view);
    target->setPositionGlobal(box);
    target->warpPositionSize();

    return true;
}

// an overlay window is drawn after the renderer resets the zoom, so its layout coordinates are screen coordinates: the middle
// of the monitor is where it belongs
void CInfinityFloatingAlgorithm::centerOverlay(SP<ITarget> target) {
    const auto WINDOW = target ? target->window() : nullptr;
    if (!WINDOW)
        return;

    const auto MONITOR = WINDOW->m_monitor.lock();
    if (!MONITOR)
        return;

    const auto SIZE = target->position().size();
    target->setPositionGlobal(CBox{MONITOR->m_position + (MONITOR->m_size - SIZE) / 2.F, SIZE});
    target->warpPositionSize();
}

bool CInfinityFloatingAlgorithm::arrangeOverlay(SP<ITarget> target) {
    const auto WINDOW = target ? target->window() : nullptr;
    if (!isOverlayWindow(WINDOW))
        return false;

    if (!(WINDOW->m_state & Desktop::View::WINDOW_STATE_PINNED)) {
        // pinning keeps the window on the monitor: it is drawn above the plane and unscaled
        if (!Config::Actions::pinWindow(Config::Actions::TOGGLE_ACTION_ENABLE, WINDOW))
            return false;
    }

    centerOverlay(target);
    remember(target);

    // it stays floating after the mode is left (it was never tiled)
    rememberFloatingBefore(target);

    return true;
}
