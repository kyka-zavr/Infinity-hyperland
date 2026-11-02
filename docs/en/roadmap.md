# Roadmap and status

[English](roadmap.md) | [Русский](../ru/roadmap.md)

## Done

- Infinity mode on a shared plane: free windows, pan, zoom (0.1x to 4x), several monitors, hit testing, drag and resize under zoom, exact return to tiled places.
- Alt-Tab window switcher: panel on every monitor, live previews, icons and names, keyboard and mouse control.
- Installer for Arch Linux: private prefix, login entry, isolated config (copy of the Hyprland config or fresh), uninstall.
- Upstream updates: the sync script, a daily check on GitHub, and the port from Hyprland 0.56.2 to 0.57.0.
- Bilingual documentation (English and Russian).

## Known issues

- Leaving Infinity: layouts other than dwindle are not checked; if windows were moved on the canvas, the tiled layout can differ from the original one.
- Windows on the canvas are not clipped to a monitor.
- Once the very first `toggle` after start did nothing; once windows were placed badly on leaving the mode with two monitors that overlapped. Neither was reproduced.
- Not tested: HiDPI (monitor scale other than 1), previews of windows with shadows or popups, X11 windows before their first frame, distributions other than Arch.
- Hyprland 0.57 reads only Lua configs; copying a config that exists only as `hyprland.conf` is refused with an explanation.

## Planned

- A PKGBUILD to install as a package.
- More testing on real hardware: HiDPI, several GPUs, TTY edge cases.
- Config-copy check for key collisions (warn and suggest free keys when your config already uses the Infinity keys).
- Ideas for later (not committed): bookmarks of places on the canvas, a mini-map, per-monitor mode switching.
- Following Hyprland releases (see [updating.md](updating.md)).
