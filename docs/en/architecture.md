# Architecture

[English](architecture.md) | [Русский](../ru/architecture.md)

How the fork is built and where it touches Hyprland. Paths are relative to `hyprland/src/` unless noted. For the history of what changed against the original use `git diff upstream main -- hyprland/`.

## Principles

- A fork, not a rewrite: Hyprland code is taken and changed (BSD 3-Clause), new logic lives in **new files**, and existing Hyprland files only get small hooks. The fewer upstream files we touch, the cheaper updates are ([updating.md](updating.md)).
- C++ like the rest of Hyprland. Config format, IPC, sockets and `HYPRLAND_*` variables are kept for compatibility; only the binary name differs (`InfinityHyprland`).
- Platform: Linux / Wayland, the same backends as Hyprland (aquamarine: DRM/KMS and a nested window).

## Infinity mode

Hyprland already makes layouts pluggable: a workspace has a tiled and a floating algorithm (`layout/algorithm/`). Infinity is a floating algorithm, `CInfinityFloatingAlgorithm` (`layout/algorithm/floating/infinity/`), derived from the default one.

- **World and view.** The algorithm keeps a *world box* for every target. What is on screen is `screen = world - view`, in layout coordinates. Panning changes the view and re-places all targets, so a window keeps its world position.
- **One plane for everything.** The view (offset and zoom) is a single shared state: every Infinity workspace on every monitor, visible or hidden, shows the same part of the plane. Instances register themselves; a pan or zoom re-places all of them. The world boxes of the windows and the view are saved when the mode is left (`storeCanvasBoxes`), so entering it again puts every window back where it was on the plane and restores the view; a workspace that was never in the mode starts from a clean plane.
- **Zoom is not part of the layout.** Windows keep their real size. The renderer scales the plane about the origin of the whole layout (not of each monitor, otherwise neighbouring monitors would show overlapping content): `screen' = screen * zoom`. In monitor-local physical pixels that is a scale by `z` followed by a translate of `monitorPosition * (z - 1) * monitorScale` (`render/Renderer.cpp`). Layer surfaces and the wallpaper are not scaled.
- **Input follows the zoom.** `CInputManager::planeCoords` maps a screen point to the plane (`screen / z`); windows are hit-tested in plane coordinates (`desktop/state/ViewHitTester.*`, with an explicit monitor, because a zoomed point can lie outside every monitor), layers and everything else in screen coordinates. Drag and resize (`layout/supplementary/DragController.cpp`) map the pointer through the plane; the monitor under a dragged window is found by its screen position. `CWindow::visibleOnMonitor` uses the plane region `[pos/z, (pos+size)/z]`.
- **Damage.** Damage is computed in unzoomed coordinates, so while the plane is zoomed the whole monitor is damaged (`output/Monitor.cpp`), and element damage is matched against the zoomed box (`render/ElementRenderer.cpp`).
- **Switching the mode** is `CAlgorithm::setInfinity` (`layout/algorithm/Algorithm.cpp`): the floating algorithm is swapped without moving placed windows; tiled windows are adopted as floating at their current geometry; on leaving, windows that were not floating before are re-tiled by the position of their centre. `CLayoutManager::infinityMsg` applies the change to the active workspace of every monitor.
- **Overlay windows.** App ids in `infinity:overlay_classes` (the settings window) are not part of the plane: they are pinned (`WINDOW_STATE_PINNED`), so the renderer draws them after it resets the zoom (unscaled, above the windows), and they are kept in the middle of their monitor. `CInputManager::planeCoords` treats a point over one of them as a screen point instead of dividing it by the zoom, and dragging is refused for them.
- **Pan** with the mouse: `CLayoutManager::panWithMouse` consumes raw pointer motion while the pan modifiers are held (relative devices freeze the cursor, absolute ones follow). **Zoom** with the wheel is handled in `keybinds/Manager.cpp::onAxisEvent`.
- Options are declared in `config/values/ConfigValues.cpp` (`infinity:*`).

## Window switcher

New module `switcher/`:

- `WindowSwitcher.{hpp,cpp}`: state and drawing. The list is built from the window history (all workspaces, most recent first). The panel is drawn every frame on the monitor the pointer is on (the one the switcher was opened on) with the normal render pass elements (`CRectPassElement`, `CTexPassElement`); previews are the windows' own surface textures, so they are live and work for hidden workspaces. Text is rendered with `renderText` and cached per font size. Opening, closing and the selection are animated (fade and slight scale, the accent slides between tiles). Committing focuses the chosen window explicitly, so a window on another workspace is switched to and raised.
- `AppInfo.{hpp,cpp}`: window class to application name and icon. Parses `.desktop` files (`Name`, localized `Name[...]`, `Icon`, `StartupWMClass`, `Exec`; hidden and `NoDisplay` entries do not match by `Exec`) and finds icons in the GTK theme, `hicolor`, Papirus, breeze, Adwaita and pixmaps; PNG and SVG are loaded with `Hyprgraphics::CImage`.
- Hooks into Hyprland: keys (`keybinds/Manager.cpp::onKeyEvent`: confirms on modifier release, takes Esc, Enter and arrows while open), mouse buttons (`managers/input/InputManager.cpp::processMouseDownNormal`), pointer motion (`mouseMoveUnified`), drawing (`render/Renderer.cpp`), and the `alttab` layout message (`config/shared/actions/ConfigActions.cpp`).

## Keyboard policy

`keyboard/KeyboardPolicy.*` sits on top of `IKeyboard::updateModifiers`, the one place every layout switch and lock change goes through. It remembers the layout and Num Lock state per window (`infinity:kb_layout_mode`, `kb_default_layout`, `num_mode`), applies them on focus changes (`window.active` event), and in "always" mode puts Num Lock back whenever it is switched off. Hooks: the `modifiers` and `keymap` keyboard events in `managers/input/InputManager.cpp`, the `kblayout` layout message in `config/shared/actions/ConfigActions.cpp`.

## Settings, palette and binds

`settings/` is a separate Qt6 program, `infinity-settings` (see [design-settings.md](design-settings.md)): `SettingsStore` (settings.json), `Theme` (palettes), `LuaGenerator` (settings + theme to `infinity.lua`), `Templates` (palette export), `Applier` (`--apply`), `Backend` (the bridge for QML) and the QML pages. The compositor knows nothing about it except the `infinity:*` options the generated Lua sets (`color_*`, keyboard, zoom) and one extension: `hl.rebind(old, new)` (`config/lua/bindings/LuaBindingsToplevel.cpp`) moves a bind to other keys and keeps its action, which is how binds from the user's own files are edited without rewriting them.

## Installer and session

`install.sh` and `installer/` (details: [installation.md](installation.md)):

- everything is installed into a private prefix (`/opt/infinity-hyprland`) so an installed Hyprland is not touched; the upstream `install` rule that created a `hyprland` symlink next to the binary is removed from `CMakeLists.txt`;
- `infinity-hyprland-session` is the login entry: it sets `XDG_CURRENT_DESKTOP=Infinity-hyprland:Hyprland`, points the compositor to `~/.config/infinity-hyprland/hyprland.lua` through `HYPRLAND_CONFIG`, and starts it via `start-hyprland --path` (which restarts in safe mode after a crash);
- `infinity-hyprland-config` creates or copies the config and never overwrites.

## Where we touch upstream files

| Area | Files |
|---|---|
| Hit testing, visibility, cursor warp | `desktop/state/ViewHitTester.*`, `desktop/view/window/Window.cpp`, `desktop/view/window/WindowPresentation.cpp` |
| Pan, drag | `layout/LayoutManager.*`, `layout/supplementary/DragController.cpp`, `layout/algorithm/Algorithm.*` |
| Keys, wheel, mouse | `keybinds/Manager.cpp`, `managers/input/InputManager.*`, `config/shared/actions/ConfigActions.cpp` |
| Rendering | `output/Monitor.cpp`, `render/Renderer.cpp`, `render/ElementRenderer.cpp`, `render/pass/RendererHintsPassElement.*` |
| Options, version string | `config/values/ConfigValues.cpp`, `helpers/SystemInfo.cpp`, `CMakeLists.txt` |
| Binds from Lua | `config/lua/bindings/LuaBindingsToplevel.cpp` (`hl.rebind`) |

New files: `layout/algorithm/floating/infinity/*`, `switcher/*`, `keyboard/*`; outside the compositor: `settings/`.

## Working with the code

- Every new file starts with `// Copyright (c) <year>, kyka-zavr.` Keep the original copyright in files taken from Hyprland and add ours next to it, never instead of it.
- Prefer adding code to new files and calling it from a one-line hook in an upstream file.
- Build with a memory limit (see [installation.md](installation.md)); check the config with `InfinityHyprland --verify-config -c <file>`; test in a nested session (`dev/run-nested.sh`, two monitors with `hyprctl output create wayland`).
