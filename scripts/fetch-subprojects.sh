#!/usr/bin/env bash
# Подтягивает подпроекты Hyprland, которых нет в этом репозитории (версии из scripts/subprojects.pins).
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
sub="$root/hyprland/subprojects"
mkdir -p "$sub"

while read -r name url rev; do
    [[ -z "$name" || "$name" == \#* ]] && continue

    if [[ -e "$sub/$name/.git" ]]; then
        if [[ "$(git -C "$sub/$name" rev-parse HEAD 2>/dev/null)" == "$rev" ]]; then
            echo "$name: уже $rev"
            continue
        fi
        rm -rf "${sub:?}/$name" # другая версия: берём заново
    fi

    rm -rf "${sub:?}/$name"
    git clone --quiet "$url" "$sub/$name"
    git -C "$sub/$name" checkout --quiet "$rev"
    echo "$name: $rev"
done < "$root/scripts/subprojects.pins"
