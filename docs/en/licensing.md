# Licensing

[English](licensing.md) | [Русский](../ru/licensing.md)

## The fork

Infinity-hyperland is a fork of Hyprland and is distributed under **BSD 3-Clause**, like Hyprland. [LICENSE](../../LICENSE) is the same text as `hyprland/LICENSE`: copyright vaxerski (2022-2026) and kyka-zavr (2026, modifications). New files carry `Copyright (c) 2026, kyka-zavr.`

This project is **not affiliated with or endorsed by** the Hyprland authors. It is called "a fork of Hyprland" as a fact; the names of the authors and the Hyprland name/logo are not used to promote it. The fork has its own name and does not use the Hyprland logo.

## What BSD 3-Clause requires

1. Keep the copyright notice, the license text and the disclaimer in the source files and in `LICENSE`.
2. In binary distributions reproduce the same in documentation or other materials (here: `LICENSE` and `THIRD_PARTY_LICENSES.md`).
3. Do not use the names of the author and contributors to endorse or promote derived products without written permission.

Allowed: use, modify and redistribute the code, including in closed and commercial products, and name the derived project differently.

## Related projects

Checked against the `LICENSE` files of the installed versions and of the subprojects (2026-11-02).

| Project | License | How it is used |
|---|---|---|
| Hyprland | BSD 3-Clause (vaxerski) | the base of the fork |
| hyprutils, aquamarine, hyprgraphics, hyprcursor, hyprwire, hyprwayland-scanner, hyprland-protocols | BSD 3-Clause (Hypr Development) | system libraries, dynamic linking (hyprwayland-scanner: build time only; hyprland-protocols: protocol XML) |
| **hyprlang** | **LGPL-3.0** | system library, **dynamic** linking |
| udis86 | BSD 2-Clause (Vivek Thampi) | subproject, built in |
| glaze (v7.2.0) | MIT (Stephen Berry) | fetched at build time when not installed |
| tracy | BSD 3-Clause | not used (profiling only) |

Notes:

- **hyprlang is LGPL-3.0.** While it is linked dynamically (`libhyprlang.so`) and the user can replace the library, our code is not restricted. If you link it **statically** or copy its code into a project (for example when integrating the compositor into another OS), the LGPL conditions apply: provide the library sources and the ability to relink.
- Files in `hyprland/protocols/` are generated from protocol XML files; each protocol has its own copyright notice, kept in the files.
- System libraries (cairo, pango, wayland, libinput, lua, ...) are neither distributed nor copied by this project.

Full texts: [THIRD_PARTY_LICENSES.md](../../THIRD_PARTY_LICENSES.md).
