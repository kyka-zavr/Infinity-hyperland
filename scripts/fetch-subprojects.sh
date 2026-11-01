#!/usr/bin/env bash
# Подтягивает подпроекты Hyprland, которых нет в этом репозитории (версии как в нашей базе).
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
sub="$root/hyprland/subprojects"

fetch() { # <имя> <url> <коммит>
    if [[ -e "$sub/$1/.git" ]]; then echo "$1: уже есть"; return; fi
    rm -rf "${sub:?}/$1"
    git clone --quiet "$2" "$sub/$1"
    git -C "$sub/$1" checkout --quiet "$3"
    echo "$1: $3"
}

mkdir -p "$sub"
fetch hyprland-protocols https://github.com/hyprwm/hyprland-protocols bd153e76f751f150a09328dbdeb5e4fab9d23622
fetch udis86             https://github.com/canihavesomecoffee/udis86 5336633af70f3917760a6d441ff02d93477b0c86
