# Design: the canvas for several monitors (release 3)

[English](design-canvas.md) | [Русский](../ru/design-canvas.md)

Status: in progress (release 3). Done: the mode is one for the session — `InfinityCanvas` (a new file) turns it on and off on every workspace of every monitor and on the workspaces that appear later, so switching a workspace or opening a window on the second monitor no longer drops out of the mode. Next: one canvas object with its own coordinates, monitors as viewports, clipping, and the pan/zoom limits.

## Problems seen in real use

1. Every monitor has its own workspace, which breaks the idea of one infinite canvas.
2. While panning and zooming there are limits at which windows disappear.
3. A parallax-like effect for a window on the second monitor when that monitor is smaller than the first.
4. A window opened on the second monitor misbehaves when Infinity is already on on the first one.
5. Several smaller bugs (to be collected while testing).

## Why they happen (from the code)

- Infinity is a *floating algorithm of a workspace*. A shared view (offset and zoom) keeps the instances in step, but every window still belongs to a workspace that belongs to **one** monitor, and workspaces are switched independently (problems 1 and 4: toggling touches only the active workspace of each monitor, a workspace that is shown later, or created later, is not in Infinity).
- Window positions live in *layout coordinates*, the same space the monitors are placed in, so the plane and the screens are mixed together. Differently sized or positioned monitors then give different results for the same plane point (problem 3 is most likely here and in the per-monitor scale factor), and the plane has the same limits as the layout space (problem 2).
- Visibility, damage and focus are decided per workspace and per monitor, so a window that is half on one monitor and half on another is a special case everywhere.

## Direction

A session-wide **canvas** object instead of per-workspace algorithms:

- The mode is global: one switch for all monitors and all workspaces, including workspaces created later.
- In Infinity every window belongs to the canvas, not to a monitor's workspace. The canvas has its own coordinate space (decoupled from the layout of the monitors), one z-order and one focus order.
- Monitors are viewports onto the canvas: each shows the part of the plane that its physical place and the shared pan and zoom select. Windows are drawn on every monitor they intersect, clipped to it.
- Tiling mode keeps working as before. A window remembers its home workspace and tiled place for the way back.
- Panning and zooming are plain transforms of the viewports; there is nothing to disappear at their limits, and no parallax because every monitor uses the same mapping from the canvas to its pixels.

## Plan

1. Collect the bugs: a reproducible scenario for each (nested session with two or three monitors of different sizes and scales).
2. Write tests first (the compositor has `hyprtester`) for the scenarios.
3. Introduce the canvas object and move placement, hit testing, drag and rendering onto it.
4. Remove the per-workspace Infinity algorithm.

Because this touches the core of the fork, it is a separate release, and the 0.57 line keeps working until then.
