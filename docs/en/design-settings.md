# Design: settings, theme and binds

[English](design-settings.md) | [Русский](../ru/design-settings.md)

Status: implemented (2026-11-02); the settings program is one binary, `infinity-settings`, which is both the window and the `--apply` command line tool (it is a Qt program, not a separate Qt Core tool). Goals: everything is customizable from one place, colors come from one palette, there is a normal settings window, and binds (ours and the system's) can be edited.

## One source of truth

Everything lives in `~/.config/infinity-hyprland/`:

| File | Purpose |
|---|---|
| `settings.json` | the settings model: Infinity, keyboard, appearance, switcher, binds, active theme |
| `themes/<name>.json` | a palette: named colors (`bg`, `surface`, `text`, `accent`, `border_active`, ...) |
| `templates/*.tmpl` | optional exports of the palette to other programs (terminal, bar, GTK...), rendered on every apply, with an optional post command |
| `infinity.lua` | **generated**, never edited by hand; included from `hyprland.lua` with one line |

`infinity-hyprland-apply` (a small Qt Core CLI) turns `settings.json` + the theme into `infinity.lua`, renders the templates and reloads the config. The settings window, the installer and the session start all call it, so there is one code path.

## Colors

All colors of everything we draw (window switcher, settings window, window borders, shadows, group bars) come from the active palette. The generated `infinity.lua` sets `infinity:color_*` options and the matching Hyprland options (`general.col.*`, `decoration.shadow.*`, ...). Changing a color in the palette changes it everywhere at once, and the templates carry it to programs outside the compositor. Presets (dark, light, custom accent) are just theme files.

## The settings window (`SUPER+I`)

A Qt6/QML application, `infinity-settings` (a C++ backend plus QML pages), started by a bind. It is an ordinary Wayland client (floating, centered), not part of the compositor. Pages:

- **Infinity**: mode, zoom, zoom step, pan modifiers.
- **Keyboard**: layouts, per-window mode, default layout for new windows, Num Lock mode.
- **Appearance**: theme and colors, borders, gaps, rounding, blur, shadows.
- **Switcher**: tile size, previews on/off, confirm key.
- **System**: volume, brightness, network (through `wpctl`, `brightnessctl`, `nmcli` when present).
- **Keybinds**: see below.

Changes are written to `settings.json`, then `infinity-hyprland-apply` is run, so they show up immediately.

## Binds

Three layers, all in `settings.json`:

1. **Managed binds**: the actions that belong to this fork (toggle Infinity, switcher, layout switch, settings, pan, zoom). Each has a default key and can be rebound.
2. **Custom binds**: the user's own (key, action: run a program or a Lua dispatch).
3. **Overrides of other binds**: the window lists *all* active binds (read from the compositor with `hyprctl binds`, including ones from hand-written Lua files) and lets you disable or rebind any of them. An override is stored as data and applied after the user's config (`hl.unbind` plus a new bind), so hand-written files are never rewritten.

## Keyboard layouts in the installer

The installer becomes a TUI (Python `curses`; keyboard first, mouse where the terminal provides it: xterm mouse reporting in terminal emulators, `gpm` in a bare TTY). Its keyboard page writes the same `settings.json` (layout list from `/usr/share/X11/xkb/rules/evdev.lst`, switching mode, default layout, Num mode) and calls `infinity-hyprland-apply`.

## Multi-monitor rework

Planned separately, after the settings work. Several multi-monitor problems are known from real use. The likely direction: windows in Infinity mode belong to the plane itself instead of a per-monitor workspace, and monitors become viewports onto it. This needs a bug list from real use first.
