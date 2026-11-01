#!/usr/bin/env bash
# Запуск собранного Hyprland вложенным окном внутри текущей Wayland-сессии.
# Конфиг ~/.config/hypr не используется: берётся dev/hyprland.lua.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
bin="$root/build/InfinityHyprland"

if [[ ! -x "$bin" ]]; then
    echo "Нет $bin. Сначала соберите проект (docs/04-roadmap.md)." >&2
    exit 1
fi
if [[ -z "${WAYLAND_DISPLAY:-}" ]]; then
    echo "Нужна запущенная Wayland-сессия (WAYLAND_DISPLAY пуст)." >&2
    exit 1
fi

log="${XDG_RUNTIME_DIR:-/tmp}/infinity-hyprland-nested.log"
echo "Лог: $log"
echo "Выход: ALT+M или закрыть окно."

# Иначе вложенный экземпляр считает себя частью внешнего.
unset HYPRLAND_INSTANCE_SIGNATURE
exec "$bin" -c "$root/dev/hyprland.lua" "$@" 2>&1 | tee "$log"
