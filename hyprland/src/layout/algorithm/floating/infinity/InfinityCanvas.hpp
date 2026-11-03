// Copyright (c) 2026, kyka-zavr.

#pragma once

#include "../../../../desktop/DesktopTypes.hpp"
#include "../../../../helpers/math/Math.hpp"
#include "../../../../helpers/memory/Memory.hpp"
#include "../../../../helpers/signal/Signal.hpp"

namespace Layout::Floating {
    // true when the window lives in a workspace that is in Infinity mode: such windows are on the canvas, not in a workspace
    bool isCanvasWindow(PHLWINDOW window);
    bool isCanvasWindow(PHLWINDOWREF window);

    // The Infinity mode belongs to the session, not to a workspace: it is applied to every workspace of every monitor,
    // including the ones that are created or shown later. This is the first step of the canvas of release 3
    // (docs/design-canvas.md): windows of all workspaces already share one plane, and switching a workspace no longer drops
    // out of the mode.
    class CInfinityCanvas {
      public:
        CInfinityCanvas();
        ~CInfinityCanvas() = default;

        bool enabled() const;

        // turns the mode on or off on every workspace that exists now, and on the workspaces that appear later
        void setEnabled(bool enabled);
        void toggle();

        // the camera: one transform for every monitor, desk = (canvas - view) * zoom. Canvas coordinates are layout
        // coordinates after the view is subtracted: screen = canvas - view, and the renderer scales that by zoom.
        Vector2D view() const;
        float    zoom() const;
        void     setView(const Vector2D& view);
        void     setZoom(float zoom);
        void     pan(const Vector2D& Δ);                       // the content follows the delta (screen pixels)
        bool     zoomBy(float mult, const Vector2D& anchor);   // keeps the layout point `anchor` fixed, false when clamped

        // the part of the canvas a monitor shows at the current zoom: [pos / zoom, (pos + size) / zoom]
        CBox visibleRect(PHLMONITOR monitor) const;

      private:
        void applyTo(PHLWORKSPACE workspace);
        void applyToAll();

        Vector2D            m_view;
        float               m_zoom = 1.F;
        bool                m_enabled = false;
        CHyprSignalListener m_wsActive, m_wsCreated;
    };

    UP<CInfinityCanvas>& canvas();
}
