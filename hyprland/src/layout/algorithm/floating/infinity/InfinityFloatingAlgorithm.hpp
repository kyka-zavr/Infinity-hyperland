// Copyright (c) 2026, kyka-zavr.

#pragma once

#include "../default/DefaultFloatingAlgorithm.hpp"

#include <map>
#include <set>
#include <vector>

namespace Layout::Floating {
    // Infinity mode: windows are free-floating boxes on an unbounded plane.
    // Every target has a "world" box; what is on screen is the world box moved by the view offset:
    //     screen = world - view
    // Panning only changes the view and re-places all targets, so a window keeps its world position.
    // The view (offset and zoom) is ONE for the whole compositor: every Infinity workspace, on any monitor, hidden or
    // visible, shows the same part of the same plane. Each algorithm only keeps the world boxes of its own targets and
    // the view it has placed them for (m_applied); a pan or zoom changes the shared view and re-places all of them.
    // Zoom is not part of the layout at all: windows keep their real size, the renderer scales the whole
    // plane about the monitor origin (screen' = screen * zoom) and input is mapped back through the same factor.
    class CInfinityFloatingAlgorithm : public CDefaultFloatingAlgorithm {
      public:
        CInfinityFloatingAlgorithm();
        virtual ~CInfinityFloatingAlgorithm();

        virtual void newTarget(SP<ITarget> target);
        virtual void movedTarget(SP<ITarget> target, std::optional<Vector2D> focalPoint = std::nullopt);
        virtual void removeTarget(SP<ITarget> target);

        virtual void resizeTarget(const Vector2D& Δ, SP<ITarget> target, eRectCorner corner = CORNER_NONE);
        virtual void moveTarget(const Vector2D& Δ, SP<ITarget> target);
        virtual void setTargetGeom(const CBox& geom, SP<ITarget> target);
        virtual void swapTargets(SP<ITarget> a, SP<ITarget> b);
        virtual void moveTargetInDirection(SP<ITarget> t, Math::eDirection dir, bool silent);
        virtual void recenter(SP<ITarget> t);

        // start tracking a target at its current on-screen position
        void adopt(SP<ITarget> target);

        // move the whole plane (all Infinity workspaces) by a screen-space delta (content follows the delta)
        void pan(const Vector2D& Δ);

        // multiply the zoom by mult keeping the on-screen point `anchor` (monitor-local) fixed
        void zoomBy(float mult, const Vector2D& anchor);

        // put only this algorithm's targets at their world positions (the view origin), used when this workspace leaves the mode.
        // The shared view itself returns to the origin when the last Infinity workspace is gone.
        void resetView();

        // targets that were floating before the mode was entered, they stay floating on exit
        void rememberFloatingBefore(SP<ITarget> target);
        bool wasFloatingBefore(SP<ITarget> target) const;

        Vector2D view() const;
        float    zoom() const;

      private:
        void remember(SP<ITarget> target);
        void applyView();
        void damageMonitor() const;

        // the view the targets of this algorithm are placed for
        Vector2D                           m_applied;

        // shared by all instances
        static Vector2D                                  s_view;
        static float                                     s_zoom;
        static std::vector<CInfinityFloatingAlgorithm*>  s_instances;
        std::map<WP<ITarget>, CBox>        m_world;
        std::set<WP<ITarget>>              m_floatingBefore;
    };
}
