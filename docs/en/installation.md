# Installation

[English](installation.md) | [Русский](../ru/installation.md)

## Requirements

- Arch Linux or a derivative (the installer uses `pacman`). Other distributions: build manually, see below.
- A GPU and driver that run Hyprland (OpenGL ES 3).
- `sudo` for installing packages and copying files into `/opt`.
- About 4 GB of free RAM for the build. The installer picks the number of parallel jobs from total memory (about 2 GB per job, at most 6 and at most the number of cores); override it with `--jobs N`.

## Install

```sh
git clone https://github.com/kyka-zavr/Infinity-hyperland.git
cd Infinity-hyperland
./install.sh
```

What it does, in order:

1. Checks the dependency list (`installer/arch-deps.txt`) and offers to install the missing packages with `pacman`.
2. Fetches the `udis86` subproject (`scripts/fetch-subprojects.sh`).
3. Builds the compositor with a memory limit (`systemd-run`, when available).
4. Installs into `/opt/infinity-hyprland` (`bin/InfinityHyprland`, `hyprctl`, `start-hyprland`, the session wrapper and the config tool).
5. Adds the login entry `/usr/share/wayland-sessions/infinity-hyprland.desktop`.
6. Asks which config to use (see below).

An already installed Hyprland is not touched: nothing is written to `/usr/bin`, `/usr/local/bin` or `~/.config/hypr`.

The installer also builds the settings window (Qt6: `qt6-base`, `qt6-declarative`, `qt6-wayland`, installed with the other packages). If your config still has the old block of Infinity binds from a previous version, the installer moves it to the settings (your changed keys are kept, a backup is made): `infinity-hyprland-config migrate`.

### Text interface

Run `./install.sh` without arguments in a terminal and a text interface opens (Python `curses`, nothing to install): config choice, install folder, build threads, **keyboard languages** (search, add several, variants), **typing** (shared or per-window layout, layout of a new window, Num Lock mode) and the color scheme. It is keyboard first (arrows, Tab, Space, Enter, Esc) and the **mouse works wherever the terminal reports it**: terminal emulators and the Linux console with the `gpm` daemon running. After the last page the window closes and the build runs in the same terminal (so `sudo` can ask for a password). Without a terminal, or with any argument, the plain installer runs as before.

### Options

| Option | Meaning |
|---|---|
| `--config copy\|fresh\|skip` | choose the config mode without asking |
| `--prefix DIR` | install somewhere else (default `/opt/infinity-hyprland`) |
| `--session-dir DIR` | where the login entry goes (default `/usr/share/wayland-sessions`) |
| `--jobs N` | build jobs (default: total memory in GB / 2, at most 6) |
| `--yes` | do not ask questions (defaults to a fresh config if none exists) |
| `--no-deps`, `--no-build` | skip installing packages / skip building |
| `--uninstall` | remove the compositor and the login entry (configs are kept) |
| `--plain` | never open the text interface |
| `--layouts us,ru,de:nodeadkeys` | keyboard layouts (`code` or `code:variant`) |
| `--kb-mode system\|per_window`, `--kb-default N`, `--num normal\|per_window\|always` | layout mode, layout of a new window (-1 = keep), Num Lock mode |
| `--theme dark\|light` | color scheme |

## Config modes

The compositor reads its own directory, `~/.config/infinity-hyprland/` (set by the session wrapper through `HYPRLAND_CONFIG`). The original `~/.config/hypr` is only ever read, never written.

- **copy**: copies `hyprland.lua` (or `hyprland.conf`) and the files it references (`require`, `dofile`, `source`, paths into `~/.config/hypr`) into the new directory, rewrites those paths to the new location, and appends the Infinity binds. Other Hyprland tools (hyprlock, hypridle, hyprpaper, ...) keep reading their files from `~/.config/hypr` as before.
- **fresh**: creates a clean config from the example (`hyprland/example/hyprland.lua`), which already contains the Infinity binds.
- An existing config is never overwritten. The config tool refuses; `--force` first makes a backup next to it (`infinity-hyprland.bak-<time>`).

Hyprland 0.57 reads only Lua configs (`hyprland.lua`). If you only have the old `hyprland.conf`, copy mode refuses with an explanation; use fresh.

The tool can be run later: `/opt/infinity-hyprland/bin/infinity-hyprland-config fresh|copy-hyprland|status`.

**Bind collisions.** The Infinity binds are appended at the end of the config, between `>>> infinity-hyprland` and `<<< infinity-hyprland`. If your config already binds the same keys, both binds fire. Typical cases: `SUPER+SHIFT+SPACE` (often a media or launcher bind) and `ALT+TAB` (if you have your own switcher). Edit or remove the block's lines or your own bind. Keys that are free in your config are easy to pick: the whole block is plain Lua.

## Start it

- **Login screen:** log out and choose **Infinity-hyprland** in the session menu (SDDM, GDM, greetd, ...).
- **From a TTY** (safest for the first run; your current session stays alive): switch to a free TTY (`CTRL+ALT+F3`), log in and run `/opt/infinity-hyprland/bin/infinity-hyprland-session`.

The session wrapper sets `XDG_CURRENT_DESKTOP=Infinity-hyprland:Hyprland` (so portals and apps that expect "Hyprland" keep working), points the compositor to your config, and starts it through `start-hyprland`, which restarts the compositor in safe mode after a crash.

## Updates inside the app

- The session checks GitHub every 6 hours and shows a notification when a new release is out. Settings (`SUPER+I`) → **About** shows the versions and has **Changelog**, **Check** and **Update** buttons (below: hardware and system information).
- **Update** opens a terminal and runs `infinity-update run` (you can also run it yourself): it clones the release tag into `~/.local/share/infinity-hyprland/src`, builds, runs `install.sh --plain --yes --config skip` and asks for the sudo password. Your configs stay. Log out and back in to start the new version.
- Libraries: `install.sh` copies the Hyprland libraries the compositor was built with into `/opt/infinity-hyprland/lib`, so a system update of `aquamarine`, `hyprutils` and similar does not break the installed version. If it still cannot start, the session logs in to the regular Hyprland and shows a notification (log: `~/.local/state/infinity-hyprland-fallback.log`).

## Uninstall

```sh
./install.sh --uninstall
```

Removes `/opt/infinity-hyprland` and the login entry. `~/.config/infinity-hyprland` stays; delete it by hand if you do not need it.

## Manual build and development

```sh
scripts/fetch-subprojects.sh
cmake -S hyprland -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j3
dev/run-nested.sh        # runs build/InfinityHyprland in a window inside your current Wayland session
```

Use a memory limit on weak machines, for example `systemd-run --user --scope -p MemoryMax=6G -p MemorySwapMax=0 nice -n 19 cmake --build build -j3`; building with `-j$(nproc)` can exhaust the memory of a whole system.

For nested runs the config is `dev/hyprland.lua` (main modifier `ALT`, window switcher on `CTRL+TAB`, because the outer session usually owns `SUPER` and `ALT+TAB`). A second monitor can be added to a nested session with `hyprctl output create wayland`.

Other distributions: install the dependencies of Hyprland 0.57 (see `installer/arch-deps.txt` for the package names on Arch), build as above, and install with `cmake --install build --prefix /your/prefix`. The files in `installer/` (session wrapper, config tool, `.desktop` entry) work on any systemd or non-systemd distribution; replace `@PREFIX@` in them.

## Troubleshooting

- **Black screen or crash at login:** switch to another TTY (`CTRL+ALT+F2`/`F3`). The compositor restarts in safe mode by itself. The log is `$XDG_RUNTIME_DIR/hypr/*/hyprland.log`.
- **A key does something else, or twice:** see "Bind collisions" above.
- **Plugins from `hyprpm` do not load:** plugins are tied to the exact Hyprland build; this fork is a different build, so they most likely will not load.
- **`hyprctl` says it cannot connect:** use `/opt/infinity-hyprland/bin/hyprctl`; a `hyprctl` from another Hyprland version may not match.
