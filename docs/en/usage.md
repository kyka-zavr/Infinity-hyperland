# Usage

[English](usage.md) | [Русский](../ru/usage.md)

## Two modes

Every workspace is in one of two modes, and the mode is switched on all monitors at once:

- **Tiling**: the usual Hyprland layout (dwindle, master, scrolling, monocle).
- **Infinity**: windows are free boxes on one unbounded plane. The screen is a view over that plane. You pan and zoom the plane; there are no borders and no "home" button (a lost window is found with the window switcher).

Switching never loses anything:

- a window created in tiling mode remembers its tiled place; after Infinity and back it returns to it;
- a window created in Infinity takes the nearest free place in the tiled layout when you switch back;
- windows that were already floating stay floating.

New windows in Infinity open in the middle of the visible area and may overlap.

With several monitors the plane is shared: monitors show neighbouring parts of the same plane, laid out like the monitors stand on your desk, so a window can be dragged from one screen to another. Pan and zoom are shared too.

## Controls

Binds below come from `hyprland/example/hyprland.lua` (section INFINITY) or from the block the installer appends to your config. The mouse actions that are built into the compositor work without any bind.

| Action | Default |
|---|---|
| Toggle Infinity | `SUPER+SHIFT+SPACE` |
| Move a window | `SUPER+LMB` drag |
| Resize a window | `SUPER+RMB` drag (also by the window edge when `general:resize_on_border` is on) |
| Pan the canvas | move the mouse with the pan modifiers held (built in, `SUPER+ALT`) |
| Pan from the keyboard | `SUPER+CTRL+arrows` (150 px per press, repeats) |
| Zoom | `CTRL+wheel` around the cursor (built in) |
| Zoom from the keyboard | `SUPER+CTRL+=` zoom in, `SUPER+CTRL+-` zoom out |
| Window switcher | `ALT+TAB`, back: `ALT+SHIFT+TAB` |

Zoom scales the whole canvas like a map; windows keep their real size. Zoom ranges from 0.1x to 4x. Panels and wallpapers (layer-shell surfaces) are not zoomed. Outside Infinity the wheel goes to the application as usual.

On a real mouse the cursor stays in place while you pan and the canvas moves under it. In a nested session (absolute pointer) the cursor follows the mouse.

## Window switcher

Hold a modifier and press `TAB`: a panel with tiles appears on every monitor, one tile per window of **all workspaces**, most recently used first. Each tile shows the application icon and name, a **live preview** and the window title. The selection starts on the previously used window.

| Key / action | Does |
|---|---|
| `TAB` / `SHIFT+TAB` | next / previous tile |
| arrows | move the selection |
| mouse hover | selects the tile under the pointer |
| left click on a tile | focus that window |
| click outside the panel | cancel |
| `ENTER` | focus the selected window |
| `ESC` | cancel |
| release the held modifier | focus the selected window |

A quick `ALT+TAB` tap switches to the previous window and a second tap comes back. In Infinity mode the canvas is panned so that the chosen window ends up in the middle of its monitor (zoom taken into account); in tiling mode its workspace is activated. The modifier that confirms the choice is the one held when the panel opened (`ALT`, `CTRL` or `SUPER`; `SHIFT` only changes direction).

## Options

```lua
hl.config({ infinity = {
    pan_mods  = "SUPER ALT",   -- modifiers that turn mouse movement into canvas panning; "" disables
    zoom_mods = "CTRL",        -- modifiers for wheel zoom; "" disables
    zoom_step = 1.1,           -- zoom factor of one wheel notch (1.01 to 2)
} })
```

## Commands

Everything is a layout message, usable from binds and from `hyprctl`:

| Message | Does |
|---|---|
| `infinity toggle` / `on` / `off` | switch the mode on all monitors |
| `infinity pan <dx> <dy>` | move the canvas; content follows the delta (pixels on screen) |
| `infinity zoom <k>` | multiply the zoom by `k` around the middle of the monitor |
| `infinity zoom <k> <x> <y>` | the same around a point (monitor-local) |
| `infinity zoom <k> cursor` | the same around the pointer |
| `infinity probe` | debug: which window is under the pointer |
| `alttab` / `alttab prev` | open the window switcher / move the selection |

```lua
hl.bind("SUPER + SHIFT + SPACE", hl.dsp.layout("infinity toggle"))
hl.bind("ALT + Tab",             hl.dsp.layout("alttab"))
hl.bind("ALT + SHIFT + Tab",     hl.dsp.layout("alttab prev"))
```

```sh
hyprctl dispatch 'hl.dsp.layout("infinity pan 200 0")'
```

Hyprland 0.57 reads only Lua configs (`hyprland.lua`); the old `hyprland.conf` format is gone upstream.

## Known limitations

- Leaving Infinity: windows that were not floating before become tiled and get a place by the position of their centre. If windows were moved on the canvas, the layout can differ from the original one. Checked with dwindle only.
- Windows on the canvas are not clipped to a monitor: a window between two monitors is drawn on both.
- HiDPI (monitor scale other than 1) is not tested.
- Hyprland plugins (`hyprpm`) are tied to a specific build and most likely will not load.
