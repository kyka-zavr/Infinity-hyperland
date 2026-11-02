#!/usr/bin/env bash
# Печатает в Markdown раздел «Versions of the sources» для описания релиза:
# база Hyprland, библиотеки, подпроекты и инструменты сборки. Версии берутся с машины, где делается релиз.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

lib_version() { # <pkg-config имя> [<имя пакета pacman>]
    local v=""
    v=$(pkg-config --modversion "$1" 2>/dev/null) || true
    [[ -z "$v" ]] && command -v pacman >/dev/null && v=$(pacman -Q "${2:-$1}" 2>/dev/null | awk '{print $2}') || true
    printf '%s' "${v:-?}"
}

tag=$(sed -n 1p UPSTREAM)
commit=$(sed -n 2p UPSTREAM)

echo "## Versions of the sources"
echo
echo "| Component | Version |"
echo "|---|---|"
echo "| **Hyprland (base)** | $tag (\`${commit:0:7}\`), https://github.com/hyprwm/Hyprland |"
echo "| Infinity-hyperland | $(git describe --tags --always 2>/dev/null || echo '?') (\`$(git rev-parse --short HEAD)\`) |"
for pair in "hyprutils" "hyprlang" "aquamarine" "hyprgraphics" "hyprcursor" "hyprwire" "hyprland-protocols"; do
    echo "| $pair | $(lib_version "$pair") |"
done
echo "| hyprwayland-scanner | $(command -v pacman >/dev/null && pacman -Q hyprwayland-scanner 2>/dev/null | awk '{sub(/-[0-9]+$/, "", $2); print $2}' || echo '?') |"
echo "| lua | $(lib_version lua) |"

# подпроекты по закреплённым коммитам
while read -r name url rev; do
    [[ -z "${name:-}" || "$name" == \#* ]] && continue
    echo "| $name (subproject) | \`${rev:0:7}\`, $url |"
done < scripts/subprojects.pins

# glaze: системный или подтягиваемый при сборке
glaze=$(pkg-config --modversion glaze 2>/dev/null || true)
[[ -z "$glaze" ]] && glaze=$(grep -oE 'set\(GLAZE_VERSION [^)]+\)' hyprland/CMakeLists.txt | awk '{print $2}' | tr -d ')')
echo "| glaze | ${glaze:-?} |"

echo "| compiler | $(c++ --version 2>/dev/null | head -1 || echo '?') |"
echo "| cmake | $(cmake --version 2>/dev/null | head -1 | awk '{print $3}') |"
echo
echo "Versions of system libraries are those on the build machine; the installer builds against whatever the distribution provides (minimums are set by Hyprland's \`CMakeLists.txt\`)."
