# Infinity-hyperland

<p align="center"><img src="assets/logo/mark-dark.svg" width="180" alt="Infinity-hyperland"></p>

[English](README.md) | [Русский](README.ru.md)

> **This is a fork of [Hyprland](https://github.com/hyprwm/Hyprland), not the original project.**
> Hyprland is written by vaxerski and the hyprwm contributors (BSD 3-Clause); the whole compositor core is their work.
> This fork is not affiliated with or endorsed by the Hyprland authors. Base: **v0.57.0** (`0aa6745`).
> Everything added on top is listed below and visible in the git history (`git diff upstream main`).

A Wayland compositor with a native **infinite canvas**: windows live on one unbounded plane, and the screen is a view that you pan and zoom over it. The idea comes from VXWM (Versatile X Window Manager, a dwm fork; [GitHub mirror](https://github.com/wh1tepearll/vxwm)); here it is built into the Hyprland core, not a plugin. The usual tiling mode stays, and you can switch between the two on the fly.

## What is added to Hyprland

- **Infinity mode.** Free windows on an unbounded plane. Switching modes loses nothing: when you leave Infinity, windows return to their tiled places.
- **Pan and zoom** (0.1x to 4x), shared by all monitors. The plane is continuous: a window can be dragged from screen to screen.
- **Alt-Tab window switcher** with live previews, application icons and names. It lists windows of all workspaces on every monitor, works in both modes, and centers the chosen window on the canvas.
- **Settings window** (`SUPER+I`): Infinity, keyboard layouts (shared or per window, Num Lock modes), one color palette for everything, and a bind editor that also handles the binds from your own config.
- **Installer** for Arch Linux and derivatives, with a text interface (mouse where the terminal supports it): installs next to an existing Hyprland without touching it, adds a login-screen entry, and keeps its own config directory.

## Controls

Binds from `hyprland/example/hyprland.lua` (main modifier `SUPER`):

| Action | Keys |
|---|---|
| Toggle Infinity on / off | `SUPER+SHIFT+SPACE` |
| Move / resize a window | `SUPER+LMB` / `SUPER+RMB` (drag) |
| Pan the whole canvas | move the mouse with `SUPER+ALT` held (or `SUPER+CTRL+arrows`) |
| Zoom | `CTRL+wheel` (or `SUPER+CTRL+=` and `SUPER+CTRL+-`) |
| Window switcher | `ALT+TAB`, back with `ALT+SHIFT+TAB`; arrows, mouse, `ENTER`, `ESC` |
| Settings | `SUPER+I` |
| Next keyboard layout | `SUPER+SPACE` |

Details: [docs/en/usage.md](docs/en/usage.md).

## Install (Arch Linux and derivatives)

```sh
git clone https://github.com/kyka-zavr/Infinity-hyperland.git
cd Infinity-hyperland
./install.sh
```

The installer checks and installs dependencies, builds the compositor (10-30 minutes), and adds an **"Infinity-hyprland"** entry to your login screen (SDDM, GDM, greetd, ...).

- It installs into `/opt/infinity-hyprland`; an installed Hyprland (`/usr/bin/Hyprland`, `hyprctl`) is **not touched**. Both can stay installed and you pick one at login.
- It uses a separate config, `~/.config/infinity-hyprland/`. You choose: **copy** the needed files from `~/.config/hypr` (the original stays untouched, the two then evolve independently) or start with a **fresh** config. An existing config is never overwritten.
- Uninstall: `./install.sh --uninstall` (configs are kept).

Full guide: [docs/en/installation.md](docs/en/installation.md).

## Status

Tested by the owner on a real Arch Linux system (login screen, real mouse and monitors) and in nested sessions. Not tested: HiDPI, non-dwindle layouts when leaving Infinity, other distributions. Known issues and plans: [docs/en/roadmap.md](docs/en/roadmap.md).

## Documentation

[English](docs/en/) | [Русский](docs/ru/): installation, usage, architecture, updates, licensing, roadmap. Release notes: [CHANGELOG.md](CHANGELOG.md).

## License

BSD 3-Clause, same as Hyprland: [LICENSE](LICENSE). Copyright (c) 2022-2026 vaxerski, (c) 2026 kyka-zavr (modifications). Third-party code and dependency licenses: [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md), [docs/en/licensing.md](docs/en/licensing.md).
