# Usage

[English](usage.md) | [Русский](../ru/usage.md)

## Two modes

Every workspace is in one of two modes. The mode belongs to the session: switching it applies to every workspace of every monitor, including workspaces created later.

- **Tiling**: the usual Hyprland layout (dwindle, master, scrolling, monocle).
- **Infinity**: windows are free boxes on one unbounded plane. The screen is a view over that plane. You pan and zoom the plane; there are no borders and no "home" button (a lost window is found with the window switcher).

Switching never loses anything:

- a window created in tiling mode remembers its tiled place; after Infinity and back it returns to it;
- a window created in Infinity takes the nearest free place in the tiled layout when you switch back;
- windows that were already floating stay floating;
- switching the mode off and on again returns the windows to the places and sizes they had on the canvas, and the plane to the same pan and zoom. A workspace that was never in the mode starts from a clean plane.

New windows in Infinity open in the middle of the visible area (at any zoom) and may overlap.

With several monitors the plane is shared: monitors show neighbouring parts of the same plane, laid out like the monitors stand on your desk, so a window can be dragged from one screen to another. Pan and zoom are shared too. In the mode the workspaces stop mattering for the picture: every monitor draws every window of the canvas that its view covers, so switching a workspace changes nothing on screen.

## Controls

Binds below come from `hyprland/example/hyprland.lua` (section INFINITY) or from the block the installer appends to your config. The mouse actions that are built into the compositor work without any bind.

| Action | Default |
|---|---|
| Toggle Infinity | `SUPER+SHIFT+SPACE` |
| Move a window | `SUPER+LMB` drag (near the edge of a monitor the canvas slides too) |
| Resize a window | `SUPER+RMB` drag (also by the window edge when `general:resize_on_border` is on) |
| Pan the canvas | move the mouse with the pan modifiers held (built in, `SUPER+ALT`) |
| Pan from the keyboard | `SUPER+CTRL+arrows` (150 px per press, repeats) |
| Zoom | `CTRL+wheel` around the cursor (built in) |
| Zoom from the keyboard | `SUPER+CTRL+=` zoom in, `SUPER+CTRL+-` zoom out |
| Window switcher | `ALT+TAB`, back: `ALT+SHIFT+TAB` |

While a window is dragged in Infinity and the pointer comes near the edge of a monitor, the canvas slides under it, so a window is never stopped by a screen border: drag it to the edge and keep dragging.

Zoom scales the whole canvas like a map; windows keep their real size. Zoom ranges from 0.1x to 4x. Panels and wallpapers (layer-shell surfaces) are not zoomed. Outside Infinity the wheel goes to the application as usual.

On a real mouse the cursor stays in place while you pan and the canvas moves under it. In a nested session (absolute pointer) the cursor follows the mouse.

## Window switcher

Hold a modifier and press `TAB`: a panel with tiles appears on the monitor the pointer is on, one tile per window of **all workspaces**, most recently used first. Each tile shows the application icon and name, a **live preview** and the window title. The selection starts on the previously used window.

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

A quick `ALT+TAB` tap switches to the previous window and a second tap comes back. The chosen window is always shown: if it lives on another workspace, that workspace (and its monitor) is switched to and the window is raised. In Infinity mode the canvas is also panned so that the window ends up in the middle of its monitor (zoom taken into account). The modifier that confirms the choice is the one held when the panel opened (`ALT`, `CTRL` or `SUPER`; `SHIFT` only changes direction).

## Options

```lua
hl.config({ infinity = {
    pan_mods        = "SUPER ALT",         -- modifiers that turn mouse movement into canvas panning; "" disables
    zoom_mods       = "CTRL",              -- modifiers for wheel zoom; "" disables
    zoom_step       = 1.1,                 -- zoom factor of one wheel notch (1.01 to 2)
    overlay_classes = "infinity-settings", -- app ids that are an overlay on the monitor in Infinity: pinned, not zoomed, centered; "" disables
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
| `kblayout next` / `prev` / `<n>` | switch the keyboard layout (see "Keyboard layouts") |

```lua
hl.bind("SUPER + SHIFT + SPACE", hl.dsp.layout("infinity toggle"))
hl.bind("ALT + Tab",             hl.dsp.layout("alttab"))
hl.bind("ALT + SHIFT + Tab",     hl.dsp.layout("alttab prev"))
```

```sh
hyprctl dispatch 'hl.dsp.layout("infinity pan 200 0")'
```

Hyprland 0.57 reads only Lua configs (`hyprland.lua`); the old `hyprland.conf` format is gone upstream.

## Settings window

`SUPER+I` opens **Infinity Settings** (a Qt6 application; also `infinity-settings` from a terminal or a launcher). In Infinity mode it is an **overlay on the monitor**, not a window on the canvas: it is pinned, always in the middle of the monitor, drawn at its normal size (the zoom does not scale it) and cannot be moved or resized. Change this with the `infinity:overlay_classes` option. Changes apply as you make them. Pages:

- **Infinity**: toggle the mode, zoom step, the keys for wheel zoom and mouse panning.
- **Keyboard**: add layouts (all xkb languages and variants), order them, choose shared or per-window layout, the layout of a new window and the Num Lock mode.
- **Appearance**: the color scheme and every color in it, plus (optionally) gaps, border size, rounding, blur and shadows.
- **Keybinds**: see below.
- **System**: volume (`wpctl`), brightness (`brightnessctl`), Wi-Fi (`nmcli`); a control is greyed out when its tool is missing.

Everything is stored in `~/.config/infinity-hyprland/settings.json`. `infinity-settings --apply` turns it into `infinity.lua` (included from your `hyprland.lua`) and reloads the compositor; `--print`, `--get <path>` and `--set <path> <json>` are there for scripts. You can edit `settings.json` by hand.

The window is a single instance: `SUPER+I` again (or `Esc`) closes it, so windows never pile up. `Esc` first leaves a text field, closes an open list or cancels a key capture, and closes the window when nothing else is active.

### Color scheme

One palette colors everything we draw: the window switcher, the settings window and, when "Manage these settings" is on, window borders and shadows. A theme is a file `themes/<name>.json` with named colors (`panel`, `bg`, `surface`, `surface_hi`, `well`, `text`, `subtext`, `accent`, `accent_text`, `border_active`, `border_inactive`, `shadow`, `danger`, `success`). Two themes are built in (`dark`, `light`); editing a color makes your own copy named `custom`, and you can add your own files.

**A palette that follows another program.** A theme file may carry an `import` section: a JSON file of another program and a map from its keys to ours. The built-in `serpantinum` theme (offered when that rice is installed) reads `~/.local/state/serpantinum/qs_colors.json` (or the file named by `QS_COLORS_JSON`) and follows it live: when the rice changes its colors (a new wallpaper, a new preset), the settings window, the window switcher and the generated config change with it; a small background watcher (`infinity-settings --watch`, started with the session) does this when the window is closed. Editing a color of such a theme turns it into a fixed copy named `custom`. Map format: `"accent": "blue"` takes the key `blue`, `"panel": "mantle:f0"` takes `mantle` with alpha `f0`.

**Export to other programs.** Put a template into `~/.config/infinity-hyprland/templates/`: a file `*.tmpl` whose first line is `@target: <path>` and whose body uses placeholders:

```
@target: ~/.config/kitty/infinity-colors.conf
foreground {{text}}
background {{bg}}
color4     {{accent}}
```

`{{name}}` gives `#rrggbb`; `{{name.nohash}}`, `{{name.rgb}}` (`92,143,255`), `{{name.rgba}}` and `{{name.hex8}}` (`#rrggbbaa`) are also available. Templates are rendered on every apply; the optional "command after applying" (for example to reload a bar) runs after them.

### Keybinds

Three kinds, all editable in the window:

1. **Binds of the fork** (toggle Infinity, window switcher, next layout, settings, pan, zoom): rebind (click the field and press the keys; global binds do not fire while capturing), disable, or reset.
2. **Your own binds**: any keys that run a program or a Lua action.
3. **Binds from your config**: the window lists every bind the running compositor knows, including the ones from your own Lua files, and lets you **disable** one or **move it to other keys**. Your files are never rewritten: the choice is saved as data in `settings.json` and applied last (`hl.unbind` / `hl.rebind`). The list shows the kind of action (`exec_cmd`, `close`, ...), not the command: the compositor does not keep the text of a command.

## Keyboard layouts

List the layouts in the usual `input.kb_layout` option and switch them with a bind (`SUPER+SPACE` in the example config). Three options decide how the switching behaves:

```lua
hl.config({ input = { kb_layout = "us,ru" } })
hl.bind("SUPER + SPACE", hl.dsp.layout("kblayout next"))   -- or "kblayout prev", "kblayout 1" (by index)

hl.config({ infinity = {
    kb_layout_mode    = "per_window",  -- "system": one layout for the whole session (usual Hyprland)
                                       -- "per_window": every window remembers its own layout, like Windows
    kb_default_layout = 0,             -- per_window: layout a NEW window starts with (0 = the first); -1 keeps the current one
    num_mode         = "normal",      -- "normal": unchanged
                                       -- "per_window": Num Lock is ON in new windows, can be toggled, remembered per window
                                       -- "always": Num Lock is always on, everywhere
} })
```

Layout and Num Lock are independent: `kb_layout_mode` and `kb_default_layout` never touch Num Lock, `num_mode` never touches the layout. In per_window mode the layout is also remembered when you change it with the usual XKB keys (for example `grp:alt_shift_toggle`), not only with `kblayout`.

## Known limitations

- Leaving Infinity: windows that were not floating before become tiled and get a place by the position of their centre. If windows were moved on the canvas, the layout can differ from the original one. Checked with dwindle only.
- Windows on the canvas are not clipped to a monitor: a window between two monitors is drawn on both.
- HiDPI (monitor scale other than 1) is not tested.
- Hyprland plugins (`hyprpm`) are tied to a specific build and most likely will not load.
