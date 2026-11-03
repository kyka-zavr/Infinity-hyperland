# Design: the canvas for several monitors (release 3)

[English](design-canvas.md) | [Русский](../ru/design-canvas.md)

Status: released in v0.57.0-infinity.3. The mode is one for the session; every monitor draws every window of the canvas that its view covers, without a workspace filter; focus does not switch workspaces; the camera (view, zoom, `visibleRect(monitor)`) lives on the canvas. Still to do: removing the per-workspace Infinity algorithm, an explicit canvas z-order, clipping at the renderer level, a narrower damage under zoom, special workspaces (scratchpads stay outside the canvas) and the `hyprtester` scenarios.

## The model

In Infinity there are **no workspaces and no per-monitor zones**. One canvas owns every window; a monitor is only a **camera** onto that canvas: its place on the desk and the shared pan and zoom decide which part of the canvas it shows. The idea of the mode is exactly that, so nothing in the mode may depend on which workspace or monitor a window "belongs" to.

- One canvas: one coordinate space, one z-order, one focus order.
- Windows are not in workspaces and not on monitors; monitors only look at the canvas.
- A window is drawn on every monitor whose camera sees it, clipped to that monitor.
- The mode is one for the session: on or off for everything at once.
- Outside the mode Hyprland works as before; a window remembers its home workspace and tiled place for the way back.

## Problems seen in real use

1. Every monitor has its own workspace, so switching a workspace changes what exists on the plane.
2. While panning and zooming there are limits at which windows disappear.
3. A parallax-like effect for a window on the second monitor when that monitor is smaller than the first.
4. A window opened on the second monitor misbehaves when Infinity is already on on the first one.
5. Several smaller bugs (to be collected while testing).

## Why they happen (from the code)

- Infinity is a *floating algorithm of a workspace*. The shared view keeps the instances in step, but every window still belongs to a workspace of **one** monitor, and only the active workspace of a monitor is drawn. That is what makes the workspace and the monitor visible at all (problems 1 and 4).
- Window positions live in *layout coordinates*, the space the monitors are placed in, so the plane and the desk are mixed together. The zoom is applied about the origin of the layout (`screen = layout * zoom`), so monitors at different offsets show canvas regions that move at different rates: the same window shifts differently on two monitors (problem 3), and strong zoom moves content out of a monitor entirely (problem 2) even though nothing is wrong with the window.
- Visibility, damage and focus are decided per workspace and per monitor.

## Direction

A session-wide **canvas** object (`CCanvas`) instead of per-workspace algorithms:

- **Windows.** Entering the mode takes every window onto the canvas (remembering its home workspace and geometry). Leaving it puts every window back: floating ones at their canvas size and place, tiled ones into the tiled layout by their centre (as today). The canvas is the only owner of the windows while the mode is on.
- **Camera.** Pan and zoom are one transform from canvas coordinates to desk coordinates: `desk = (canvas - view) * zoom`. Each monitor sees the canvas rectangle under its desk box; zoom keeps the canvas point under the cursor fixed for every monitor, because every monitor uses the same transform.
- **Rendering.** Every monitor draws all canvas windows that intersect its rectangle, clipped to it, in the canvas z-order. No workspace filter at all. (First version done: rendering all canvas windows, no workspace filter; clipping and the explicit canvas z-order are still to do.)
- **Input and focus.** Hit testing in canvas coordinates; focusing or raising a window does not switch a workspace (there is none), the window is already on the canvas.
- **Tiling mode** keeps working as before and is untouched.

## Plan

1. `CCanvas` object: registry of windows (home workspace + home geometry), camera (view, zoom), z-order and focus order; `canvasToDesk`, `visibleCanvasRect(monitor)`, `canvasWindowAt(point)`.
2. Rendering: in Infinity draw the canvas instead of "the active workspace of this monitor"; clip to the monitor; damage from the camera transform.
3. Input: hit test, drag/resize and focus against the canvas; drop the workspace switches.
4. Remove the per-workspace Infinity algorithm and the `InfinityCanvas` workaround; the canvas replaces both.
5. Tests: `hyprtester` scenarios for the five problems above — two monitors of different sizes and offsets, a window straddling the border, pan and zoom limits, a window opened on the second monitor.

Because this touches the core of the fork, it is a separate release; the 0.57 line keeps working until then.
