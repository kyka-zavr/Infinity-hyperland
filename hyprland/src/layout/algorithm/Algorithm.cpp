#include "Algorithm.hpp"

#include "FloatingAlgorithm.hpp"
#include "TiledAlgorithm.hpp"
#include "floating/default/DefaultFloatingAlgorithm.hpp"
#include "floating/infinity/InfinityFloatingAlgorithm.hpp"
#include "../target/WindowTarget.hpp"
#include "../space/Space.hpp"
#include "../../desktop/view/window/Window.hpp"
#include "../../desktop/history/WindowHistoryTracker.hpp"
#include "../../desktop/state/FocusState.hpp"
#include "../../output/Monitor.hpp"
#include "../../render/Renderer.hpp"

#include "../../debug/log/Logger.hpp"

using namespace Layout;

SP<CAlgorithm> CAlgorithm::create(UP<ITiledAlgorithm>&& tiled, UP<IFloatingAlgorithm>&& floating, SP<CSpace> space) {
    auto algo                  = SP<CAlgorithm>(new CAlgorithm(std::move(tiled), std::move(floating), space));
    algo->m_self               = algo;
    algo->m_tiled->m_parent    = algo;
    algo->m_floating->m_parent = algo;
    return algo;
}

CAlgorithm::CAlgorithm(UP<ITiledAlgorithm>&& tiled, UP<IFloatingAlgorithm>&& floating, SP<CSpace> space) :
    m_tiled(std::move(tiled)), m_floating(std::move(floating)), m_space(space) {
    ;
}

void CAlgorithm::addTarget(SP<ITarget> target) {
    // in Infinity mode nothing is tiled: new windows are free-floating on the plane
    if (infinityActive() && !target->floating())
        target->setFloating(true);

    const bool SHOULD_FLOAT = target->floating();

    if (SHOULD_FLOAT) {
        m_floatingTargets.emplace_back(target);
        m_floating->newTarget(target);
    } else {
        m_tiledTargets.emplace_back(target);
        m_tiled->newTarget(target);
    }
}

void CAlgorithm::removeTarget(SP<ITarget> target) {
    const bool IS_FLOATING = std::ranges::contains(m_floatingTargets, target);

    if (IS_FLOATING) {
        std::erase(m_floatingTargets, target);
        m_floating->removeTarget(target);
        return;
    }

    const bool IS_TILED = std::ranges::contains(m_tiledTargets, target);

    if (IS_TILED) {
        std::erase(m_tiledTargets, target);
        m_tiled->removeTarget(target);
        return;
    }

    LOG(Log::ERR, "BUG THIS: CAlgorithm::removeTarget, but not found");
}

void CAlgorithm::moveTarget(SP<ITarget> target, std::optional<Vector2D> focalPoint, bool reposition) {
    // a window moved onto an Infinity workspace becomes free-floating too, except when leaving the mode (setInfinity)
    if (infinityActive() && !m_leavingInfinity && !target->floating())
        target->setFloating(true);

    const bool SHOULD_FLOAT = target->floating();

    if (SHOULD_FLOAT) {
        m_floatingTargets.emplace_back(target);
        if (reposition)
            m_floating->newTarget(target);
        else
            m_floating->movedTarget(target, focalPoint);
    } else {
        m_tiledTargets.emplace_back(target);
        if (reposition)
            m_tiled->newTarget(target);
        else
            m_tiled->movedTarget(target, focalPoint);
    }
}

SP<CSpace> CAlgorithm::space() const {
    return m_space.lock();
}

void CAlgorithm::setFloating(SP<ITarget> target, bool floating, bool reposition) {
    removeTarget(target);

    g_pHyprRenderer->damageWindow(target->window());

    target->setFloating(floating);

    moveTarget(target, std::nullopt, reposition);

    g_pHyprRenderer->damageWindow(target->window());
}

size_t CAlgorithm::tiledTargets() const {
    return m_tiledTargets.size();
}

size_t CAlgorithm::floatingTargets() const {
    return m_floatingTargets.size();
}

void CAlgorithm::recalculate(eRecalculateReason reason) {
    m_tiled->recalculate(reason);
    m_floating->recalculate(reason);
}

void CAlgorithm::recenter(SP<ITarget> t) {
    if (t->floating())
        m_floating->recenter(t);
}

bool CAlgorithm::infinityActive() const {
    return dynamic_cast<Floating::CInfinityFloatingAlgorithm*>(m_floating.get());
}

// replaces the floating algorithm without letting the new one move already placed windows
void CAlgorithm::swapFloatingAlgo(UP<IFloatingAlgorithm>&& algo) {
    std::vector<std::pair<SP<ITarget>, CBox>> placed;
    for (const auto& t : m_floatingTargets) {
        if (const auto TARGET = t.lock(); TARGET)
            placed.emplace_back(TARGET, TARGET->position());
    }

    updateFloatingAlgo(std::move(algo));

    for (const auto& [target, box] : placed) {
        if (target->position() == box)
            continue;

        target->setPositionGlobal(box);
        target->warpPositionSize();
    }
}

void CAlgorithm::setInfinity(bool on) {
    if (on == infinityActive())
        return;

    if (on) {
        std::vector<std::pair<SP<ITarget>, CBox>> tiled;
        for (const auto& t : m_tiledTargets) {
            const auto TARGET = t.lock();
            if (!TARGET)
                continue;

            // the tile box includes gaps and borders: keep the window's real geometry so nothing jumps
            const auto WINDOW = TARGET->window();
            const auto BOX    = WINDOW ? CBox{WINDOW->position(Desktop::View::IGeometric::GEOMETRIC_GOAL), WINDOW->size(Desktop::View::IGeometric::GEOMETRIC_GOAL)} : TARGET->position();

            tiled.emplace_back(TARGET, BOX);
        }

        auto  algo = makeUnique<Floating::CInfinityFloatingAlgorithm>();
        auto* raw  = algo.get();

        for (const auto& t : m_floatingTargets) {
            if (const auto TARGET = t.lock(); TARGET)
                raw->rememberFloatingBefore(TARGET);
        }

        swapFloatingAlgo(std::move(algo));

        // tiled windows stay exactly where they are, they just become free
        for (const auto& [target, box] : tiled) {
            setFloating(target, true);
            target->setPositionGlobal(box);
            target->warpPositionSize();
            raw->adopt(target);
        }

        return;
    }

    auto* infinity = dynamic_cast<Floating::CInfinityFloatingAlgorithm*>(m_floating.get());
    infinity->resetView();

    std::vector<SP<ITarget>> toTile;
    for (const auto& t : m_floatingTargets) {
        if (const auto TARGET = t.lock(); TARGET && !infinity->wasFloatingBefore(TARGET))
            toTile.emplace_back(TARGET);
    }

    // the closest free place in the tiled layout is picked from where the window was
    m_leavingInfinity = true;
    for (const auto& target : toTile) {
        const auto FOCAL = target->position().middle();

        removeTarget(target);
        g_pHyprRenderer->damageWindow(target->window());
        target->setFloating(false);
        moveTarget(target, FOCAL, false);
        g_pHyprRenderer->damageWindow(target->window());
    }
    m_leavingInfinity = false;

    swapFloatingAlgo(makeUnique<Floating::CDefaultFloatingAlgorithm>());
}

void CAlgorithm::panInfinity(const Vector2D& Δ) {
    if (auto* infinity = dynamic_cast<Floating::CInfinityFloatingAlgorithm*>(m_floating.get()); infinity)
        infinity->pan(Δ);
}

void CAlgorithm::zoomInfinity(float mult, std::optional<Vector2D> anchor) {
    auto* infinity = dynamic_cast<Floating::CInfinityFloatingAlgorithm*>(m_floating.get());
    if (!infinity)
        return;

    // the anchor is given monitor-local (default: the middle of the monitor), the zoom works in layout coordinates
    const auto MONITOR = m_space.lock() && m_space.lock()->workspace() ? m_space.lock()->workspace()->m_monitor.lock() : nullptr;
    infinity->zoomBy(mult, anchor.value_or(MONITOR ? MONITOR->m_size / 2.F : Vector2D{}) + (MONITOR ? MONITOR->m_position : Vector2D{}));
}

float CAlgorithm::infinityZoom() const {
    if (const auto* infinity = dynamic_cast<const Floating::CInfinityFloatingAlgorithm*>(m_floating.get()); infinity)
        return infinity->zoom();
    return 1.F;
}

Config::ErrorResult CAlgorithm::infinityMsg(const std::string_view& sv) {
    // sv is the part after "infinity"
    const auto ARGS = std::string{sv.substr(sv.find_first_not_of(' ') == std::string_view::npos ? sv.size() : sv.find_first_not_of(' '))};

    if (ARGS == "toggle") {
        setInfinity(!infinityActive());
        return {};
    }
    if (ARGS == "on" || ARGS == "off") {
        setInfinity(ARGS == "on");
        return {};
    }
    if (ARGS.starts_with("pan ")) {
        if (!infinityActive())
            return Config::configError("infinity pan: the mode is not active on this workspace", Config::eConfigErrorLevel::WARNING, Config::eConfigErrorCode::INVALID_STATE);

        double x = 0, y = 0;
        if (std::sscanf(ARGS.c_str() + 4, "%lf %lf", &x, &y) != 2)
            return Config::configError("infinity pan: expected two numbers", Config::eConfigErrorLevel::ERROR, Config::eConfigErrorCode::INVALID_ARGUMENT);

        panInfinity({x, y});
        return {};
    }

    if (ARGS.starts_with("zoom ")) {
        if (!infinityActive())
            return Config::configError("infinity zoom: the mode is not active on this workspace", Config::eConfigErrorLevel::WARNING, Config::eConfigErrorCode::INVALID_STATE);

        double mult = 0, ax = 0, ay = 0;
        const int N = std::sscanf(ARGS.c_str() + 5, "%lf %lf %lf", &mult, &ax, &ay);
        if ((N != 1 && N != 3) || mult <= 0)
            return Config::configError("infinity zoom: expected <multiplier> [<anchor x> <anchor y>]", Config::eConfigErrorLevel::ERROR, Config::eConfigErrorCode::INVALID_ARGUMENT);

        zoomInfinity(mult, N == 3 ? std::optional<Vector2D>{{ax, ay}} : std::nullopt);
        return {};
    }

    return Config::configError("infinity: unknown command, expected toggle, on, off, pan <dx> <dy> or zoom <multiplier> [<ax> <ay>]", Config::eConfigErrorLevel::ERROR,
                               Config::eConfigErrorCode::INVALID_ARGUMENT);
}

Config::ErrorResult CAlgorithm::layoutMsg(const std::string_view& sv) {
    if (sv == "infinity" || sv.starts_with("infinity "))
        return infinityMsg(sv.substr(sizeof("infinity") - 1));

    if (const auto ret = m_floating->layoutMsg(sv); !ret)
        return ret;
    return m_tiled->layoutMsg(sv);
}

std::optional<Vector2D> CAlgorithm::predictSizeForNewTiledTarget() {
    return m_tiled->predictSizeForNewTarget();
}

void CAlgorithm::resizeTarget(const Vector2D& Δ, SP<ITarget> target, eRectCorner corner) {
    if (target->floating())
        m_floating->resizeTarget(Δ, target, corner);
    else
        m_tiled->resizeTarget(Δ, target, corner);
}

void CAlgorithm::moveTarget(const Vector2D& Δ, SP<ITarget> target) {
    if (target->floating())
        m_floating->moveTarget(Δ, target);
}

void CAlgorithm::swapTargets(SP<ITarget> a, SP<ITarget> b) {
    auto swapFirst = [&a, &b](std::vector<WP<ITarget>>& targets) -> bool {
        auto ia = std::ranges::find(targets, a);
        auto ib = std::ranges::find(targets, b);

        if (ia != std::ranges::end(targets) && ib != std::ranges::end(targets)) {
            std::iter_swap(ia, ib);
            return true;
        } else if (ia != std::ranges::end(targets))
            *ia = b;
        else if (ib != std::ranges::end(targets))
            *ib = a;

        return false;
    };

    if (!swapFirst(m_tiledTargets))
        swapFirst(m_floatingTargets);

    const WP<IModeAlgorithm> algA = a->floating() ? WP<IModeAlgorithm>(m_floating) : WP<IModeAlgorithm>(m_tiled);
    const WP<IModeAlgorithm> algB = b->floating() ? WP<IModeAlgorithm>(m_floating) : WP<IModeAlgorithm>(m_tiled);

    algA->swapTargets(a, b);
    if (algA != algB)
        algB->swapTargets(b, a);
}

void CAlgorithm::moveTargetInDirection(SP<ITarget> t, Math::eDirection dir, bool silent) {
    if (t->floating())
        m_floating->moveTargetInDirection(t, dir, silent);
    else
        m_tiled->moveTargetInDirection(t, dir, silent);
}

void CAlgorithm::updateFloatingAlgo(UP<IFloatingAlgorithm>&& algo) {
    algo->m_parent = m_self;

    const auto FOCUSED_WINDOW = Desktop::focusState()->window();
    const auto FOCUSED_TARGET = FOCUSED_WINDOW ? FOCUSED_WINDOW->layoutTarget() : nullptr;

    for (const auto& t : m_floatingTargets) {
        const auto TARGET = t.lock();
        if (!TARGET)
            continue;

        // Unhide windows when switching layouts to prevent them from being permanently lost
        const auto WINDOW = TARGET->window();
        if (WINDOW)
            WINDOW->setHidden(false);

        m_floating->removeTarget(TARGET);
        algo->newTarget(TARGET);
    }

    if (FOCUSED_TARGET && FOCUSED_TARGET->space() == m_space && FOCUSED_TARGET->floating())
        Desktop::focusState()->fullWindowFocus(FOCUSED_WINDOW, Desktop::eFocusReason::FOCUS_REASON_DESKTOP_STATE_CHANGE);

    m_floating = std::move(algo);
}

void CAlgorithm::updateTiledAlgo(UP<ITiledAlgorithm>&& algo) {
    algo->m_parent = m_self;

    const auto FOCUSED_WINDOW = Desktop::focusState()->window();
    const auto FOCUSED_TARGET = FOCUSED_WINDOW ? FOCUSED_WINDOW->layoutTarget() : nullptr;

    for (const auto& t : m_tiledTargets) {
        const auto TARGET = t.lock();
        if (!TARGET)
            continue;

        // Unhide windows when switching layouts to prevent them from being permanently lost
        // This is a safeguard for layouts (including third-party plugins) that use setHidden
        const auto WINDOW = TARGET->window();
        if (WINDOW)
            WINDOW->setHidden(false);

        m_tiled->removeTarget(TARGET);
        algo->newTarget(TARGET);
    }

    if (FOCUSED_TARGET && FOCUSED_TARGET->space() == m_space && !FOCUSED_TARGET->floating())
        Desktop::focusState()->fullWindowFocus(FOCUSED_WINDOW, Desktop::eFocusReason::FOCUS_REASON_DESKTOP_STATE_CHANGE);

    m_tiled = std::move(algo);
}

const UP<ITiledAlgorithm>& CAlgorithm::tiledAlgo() const {
    return m_tiled;
}

const UP<IFloatingAlgorithm>& CAlgorithm::floatingAlgo() const {
    return m_floating;
}

SP<ITarget> CAlgorithm::getNextCandidate(SP<ITarget> old) {
    static auto FOCUSONCLOSE = CConfigValue<Hyprlang::INT>("input:focus_on_close");

    if (old->floating() || *FOCUSONCLOSE == 2) {
        // use window history to determine best target
        for (const auto& w : Desktop::History::windowTracker()->fullHistory() | std::views::reverse) {
            if (!w->m_workspace || w->m_workspace->space() != m_space || !w->layoutTarget() || !w->layoutTarget()->space())
                continue;

            return w->layoutTarget();
        }

        // no history, fall back
    } else {
        // ask the layout
        const auto CANDIDATE = m_tiled->getNextCandidate(old);
        if (CANDIDATE)
            return CANDIDATE;

        // no candidate, fall back
    }

    // fallback: try to focus anything
    if (!m_tiledTargets.empty())
        return m_tiledTargets.back().lock();
    if (!m_floatingTargets.empty())
        return m_floatingTargets.back().lock();

    // god damn it, maybe empty?
    return nullptr;
}

void CAlgorithm::setTargetGeom(const CBox& box, SP<ITarget> target) {
    if (!target->floating() || !std::ranges::contains(m_floatingTargets, target))
        return;

    m_floating->setTargetGeom(box, target);
}
