#!/usr/bin/env bash
# Установщик Infinity-hyprland для Arch Linux и производных.
#
# Ставит композитор В ОТДЕЛЬНЫЙ каталог (по умолчанию /opt/infinity-hyprland), поэтому установленный Hyprland
# (/usr/bin/Hyprland, hyprctl и т.д.) не затрагивается. Добавляет пункт «Infinity-hyprland» в экран входа.
# Конфиги живут в отдельном каталоге ~/.config/infinity-hyprland, ~/.config/hypr не меняется.
#
#   ./install.sh                  интерактивная установка
#   ./install.sh --config copy    копию конфига Hyprland в свой каталог (copy | fresh | skip)
#   ./install.sh --uninstall      удалить композитор и пункт входа (конфиги остаются)
#   ./install.sh --help
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# без параметров в обычном терминале (в том числе в TTY) открывается текстовый интерфейс, он умеет мышь там, где её даёт терминал
if [[ $# -eq 0 && -t 0 && -t 1 && "${TERM:-dumb}" != "dumb" && -f "$ROOT/installer/tui.py" ]] \
    && command -v python3 >/dev/null && python3 -c 'import curses' 2>/dev/null; then
    exec python3 "$ROOT/installer/tui.py"
fi
PREFIX="/opt/infinity-hyprland"
SESSION_DIR="/usr/share/wayland-sessions"
CONFIG_MODE=""      # copy | fresh | skip, пусто = спросить
ASSUME_YES=0
DO_DEPS=1
DO_BUILD=1
UNINSTALL=0
JOBS=""
THEME=""; KB_MODE=""; KB_DEFAULT=""; NUM=""; LAYOUTS=""   # выбор из текстового интерфейса (или командной строки)

# зависимости сборки и работы (общий список с CI)
mapfile -t DEPS < <(grep -v '^#' "$ROOT/installer/arch-deps.txt" | tr -s ' \n' '\n\n' | grep .)

say()  { printf '\033[1m==>\033[0m %s\n' "$*"; }
warn() { printf '\033[33m!!\033[0m %s\n' "$*" >&2; }
die()  { printf '\033[31mОшибка:\033[0m %s\n' "$*" >&2; exit 1; }

usage() { sed -n '2,11p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; }

while (($#)); do
    case "$1" in
        --prefix) PREFIX="${2:?}"; shift ;;
        --session-dir) SESSION_DIR="${2:?}"; shift ;;
        --config) CONFIG_MODE="${2:?}"; shift ;;
        --jobs) JOBS="${2:?}"; shift ;;
        --yes|-y) ASSUME_YES=1 ;;
        --plain) ;;                                   # без текстового интерфейса (его запускает сам интерфейс)
        --theme) THEME="${2:?}"; shift ;;             # dark | light
        --kb-mode) KB_MODE="${2:?}"; shift ;;         # system | per_window
        --kb-default) KB_DEFAULT="${2:?}"; shift ;;   # номер раскладки нового окна, -1: текущая
        --num) NUM="${2:?}"; shift ;;               # normal | per_window | always
        --layouts) LAYOUTS="${2:?}"; shift ;;         # us,ru,de:nodeadkeys
        --no-deps) DO_DEPS=0 ;;
        --no-build) DO_BUILD=0 ;;
        --uninstall) UNINSTALL=1 ;;
        -h|--help) usage; exit 0 ;;
        *) die "неизвестный аргумент: $1 (см. --help)" ;;
    esac
    shift
done

[[ $EUID -ne 0 ]] || die "запускайте от обычного пользователя, sudo будет вызван только там, где нужно"
[[ "$PREFIX" == /* ]] || die "--prefix должен быть абсолютным путём"
case "$CONFIG_MODE" in ""|copy|fresh|skip) ;; *) die "--config: copy, fresh или skip" ;; esac

# sudo нужен, только если куда писать нельзя без него
need_root_for() { # <путь> -> 0, если нужен sudo
    local p="$1"
    while [[ ! -e "$p" ]]; do p="$(dirname "$p")"; done
    [[ ! -w "$p" ]]
}
run_priv() { if need_root_for "$1"; then sudo "${@:2}"; else "${@:2}"; fi; } # run_priv <целевой путь> <команда...>

confirm() { # <вопрос> -> 0/1
    [[ $ASSUME_YES -eq 1 ]] && return 0
    local a; read -r -p "$1 [Y/n] " a; [[ -z "$a" || "$a" =~ ^[YyДд] ]]
}

if [[ $UNINSTALL -eq 1 ]]; then
    say "Удаление Infinity-hyprland"
    [[ -d "$PREFIX" || -f "$SESSION_DIR/infinity-hyprland.desktop" ]] || { echo "Нечего удалять."; exit 0; }
    confirm "Удалить $PREFIX и пункт входа?" || exit 1
    [[ -f "$SESSION_DIR/infinity-hyprland.desktop" ]] && run_priv "$SESSION_DIR/infinity-hyprland.desktop" rm -f "$SESSION_DIR/infinity-hyprland.desktop"
    [[ -d "$PREFIX" ]] && run_priv "$PREFIX" rm -rf "$PREFIX"
    echo "Готово. Конфиги (~/.config/infinity-hyprland) и обычный Hyprland не затронуты."
    exit 0
fi

# --- установка или обновление ---------------------------------------------------------------
NEW_VERSION="$(cat "$ROOT/hyprland/VERSION" 2>/dev/null || echo '?')-infinity $(git -C "$ROOT" describe --tags --always 2>/dev/null || echo)"
PREV_VERSION=""
IS_OURS=0   # наш каталог: есть метка или файлы, которые кладёт только этот установщик (установка старым установщиком метки не имеет)
if [[ -f "$PREFIX/.infinity-hyprland" || -x "$PREFIX/bin/infinity-hyprland-session" ]]; then
    IS_OURS=1
    PREV_VERSION="$(sed -n 's/^version=//p' "$PREFIX/.infinity-hyprland" 2>/dev/null | head -1 || true)"
    [[ -n "$PREV_VERSION" ]] || PREV_VERSION="(более старая установка, версия неизвестна)"
fi
if [[ -n "$PREV_VERSION" ]]; then
    say "Обновление: $PREV_VERSION -> $NEW_VERSION (каталог $PREFIX будет заменён целиком, конфиги остаются)"
else
    say "Новая установка: $NEW_VERSION"
fi

command -v pacman >/dev/null || warn "pacman не найден: установщик рассчитан на Arch. Зависимости придётся поставить вручную (--no-deps)."

# --- зависимости ---------------------------------------------------------------------------------
if [[ $DO_DEPS -eq 1 ]] && command -v pacman >/dev/null; then
    say "Проверка зависимостей"
    mapfile -t MISSING < <(pacman -T "${DEPS[@]}" || true)
    if ((${#MISSING[@]})); then
        echo "Не хватает: ${MISSING[*]}"
        confirm "Установить через pacman?" || die "без зависимостей не собрать"
        sudo pacman -S --needed --noconfirm "${MISSING[@]}"
    else
        echo "Всё на месте."
    fi
fi

# --- сборка -------------------------------------------------------------------------------------
if [[ -z "$JOBS" ]]; then
    # сборка Hyprland занимает около 2 ГБ памяти на поток: считаем по общей памяти, не больше 6 потоков и не больше числа ядер
    mem_gb=$(awk '/MemTotal/ {printf "%d", $2/1048576}' /proc/meminfo)
    JOBS=$(( mem_gb / 2 )); (( JOBS < 1 )) && JOBS=1
    (( JOBS > $(nproc) )) && JOBS=$(nproc)
    (( JOBS > 6 )) && JOBS=6
fi

BUILD="$ROOT/build"
if [[ $DO_BUILD -eq 1 ]]; then
    say "Подпроекты"
    "$ROOT/scripts/fetch-subprojects.sh"
fi

# префикс записывается в сборку при конфигурации: всегда конфигурируем с нужным, чтобы ничего не ушло в /usr/local
say "Конфигурация"
cmake -S "$ROOT/hyprland" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX" >/dev/null

if [[ $DO_BUILD -eq 1 ]]; then
    say "Сборка (потоков: $JOBS)"
    if command -v systemd-run >/dev/null; then
        # лимит памяти, чтобы сборка не положила систему
        limit_mb=$(( JOBS * 2048 + 2048 ))
        total_mb=$(awk '/MemTotal/ {printf "%d", $2/1024}' /proc/meminfo)
        (( limit_mb > total_mb * 85 / 100 )) && limit_mb=$(( total_mb * 85 / 100 ))
        systemd-run --user --scope -q -p MemoryMax="${limit_mb}M" -p MemorySwapMax=0 nice -n 19 cmake --build "$BUILD" -j"$JOBS"
    else
        nice -n 19 cmake --build "$BUILD" -j"$JOBS"
    fi
fi
[[ -x "$BUILD/InfinityHyprland" ]] || die "нет $BUILD/InfinityHyprland, сборка не выполнена"

# --- установка ----------------------------------------------------------------------------------
say "Установка в $PREFIX"
# ставим от своего имени в промежуточный каталог и копируем в префикс: sudo не пишет в каталог сборки
# (иначе там остаётся файл root, о который спотыкается следующий запуск)
STAGE="$(mktemp -d)"
DESTDIR="$STAGE" cmake --install "$BUILD" --prefix "$PREFIX" >/dev/null
# при обновлении наш каталог (на нём метка) заменяется целиком, чтобы не оставались файлы прошлой версии
if [[ $IS_OURS -eq 1 ]]; then
    run_priv "$PREFIX" rm -rf "$PREFIX"
fi
run_priv "$PREFIX" mkdir -p "$PREFIX"
run_priv "$PREFIX" cp -a "$STAGE$PREFIX/." "$PREFIX/"
rm -rf "$STAGE"
printf 'version=%s\ninstalled=%s\n' "$NEW_VERSION" "$(date -Iseconds)" > "$STAGE.marker"
run_priv "$PREFIX" install -Dm644 "$STAGE.marker" "$PREFIX/.infinity-hyprland"
rm -f "$STAGE.marker"

say "Окно настроек (Qt6)"
SBUILD="$ROOT/build-settings"
cmake -S "$ROOT/settings" -B "$SBUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build "$SBUILD" -j"$JOBS"
run_priv "$PREFIX" install -Dm755 "$SBUILD/infinity-settings" "$PREFIX/bin/infinity-settings"

subst() { sed "s|@PREFIX@|$PREFIX|g" "$1"; }
tmp="$(mktemp -d)"; trap 'rm -rf "$tmp"' EXIT
subst "$ROOT/installer/infinity-hyprland-session" > "$tmp/infinity-hyprland-session"
subst "$ROOT/installer/infinity-hyprland-config"  > "$tmp/infinity-hyprland-config"
subst "$ROOT/installer/infinity-hyprland.desktop" > "$tmp/infinity-hyprland.desktop"

run_priv "$PREFIX" install -Dm755 "$tmp/infinity-hyprland-session" "$PREFIX/bin/infinity-hyprland-session"
run_priv "$PREFIX" install -Dm755 "$tmp/infinity-hyprland-config"  "$PREFIX/bin/infinity-hyprland-config"
run_priv "$PREFIX" install -Dm644 "$ROOT/installer/share/infinity-include.lua" "$PREFIX/share/infinity-hyprland/infinity-include.lua"

say "Пункт «Infinity-hyprland» в экране входа"
run_priv "$SESSION_DIR" install -Dm644 "$tmp/infinity-hyprland.desktop" "$SESSION_DIR/infinity-hyprland.desktop"

# --- конфиг пользователя ------------------------------------------------------------------------
CONF_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/infinity-hyprland"
CFG="$PREFIX/bin/infinity-hyprland-config"

if [[ -z "$CONFIG_MODE" ]]; then
    if [[ -e "$CONF_DIR/hyprland.lua" ]]; then
        CONFIG_MODE=skip
        echo "В $CONF_DIR уже есть конфиг, он остаётся как есть."
        INFINITY_HYPRLAND_CONFIG_DIR="$CONF_DIR" "$CFG" migrate
    elif [[ $ASSUME_YES -eq 1 ]]; then
        CONFIG_MODE=fresh
    else
        echo
        echo "Какой конфиг использовать для Infinity-hyprland?"
        echo "  1) Конфиги Hyprland: скопировать нужные файлы из ~/.config/hypr в $CONF_DIR"
        echo "     (оригинал не меняется, дальше две независимые копии)"
        echo "  2) Свои: создать чистый конфиг в $CONF_DIR и настраивать самому"
        echo "  3) Пропустить (создам сам)"
        read -r -p "Выбор [1/2/3]: " ans
        case "$ans" in 1) CONFIG_MODE=copy ;; 2) CONFIG_MODE=fresh ;; *) CONFIG_MODE=skip ;; esac
    fi
fi

# конфиг уже есть (повторная установка или обновление): его не трогаем, только переводим старый блок на настройки
if [[ "$CONFIG_MODE" == copy || "$CONFIG_MODE" == fresh ]] && [[ -e "$CONF_DIR/hyprland.lua" ]]; then
    echo "В $CONF_DIR уже есть конфиг, он остаётся как есть."
    INFINITY_HYPRLAND_CONFIG_DIR="$CONF_DIR" "$CFG" migrate
    CONFIG_MODE=skip
fi

case "$CONFIG_MODE" in
    copy)  INFINITY_HYPRLAND_CONFIG_DIR="$CONF_DIR" "$CFG" copy-hyprland ;;
    fresh) INFINITY_HYPRLAND_CONFIG_DIR="$CONF_DIR" "$CFG" fresh ;;
    skip)  ;;
esac

# --- выбор из интерфейса: тема и раскладки пишутся в настройки --------------------------------
if [[ -n "$THEME$KB_MODE$KB_DEFAULT$NUM$LAYOUTS" ]]; then
    say "Тема и раскладки"
    S_BIN="$PREFIX/bin/infinity-settings"
    export INFINITY_HYPRLAND_CONFIG_DIR="$CONF_DIR"
    "$S_BIN" --init >/dev/null
    [[ -n "$THEME" ]]      && "$S_BIN" --set theme "\"$THEME\""
    [[ -n "$KB_MODE" ]]    && "$S_BIN" --set keyboard.mode "\"$KB_MODE\""
    [[ -n "$KB_DEFAULT" ]] && "$S_BIN" --set keyboard.default_layout "$KB_DEFAULT"
    [[ -n "$NUM" ]]       && "$S_BIN" --set keyboard.num_mode "\"$NUM\""
    if [[ -n "$LAYOUTS" ]]; then
        json="["
        IFS=, read -ra L <<< "$LAYOUTS"
        for e in "${L[@]}"; do
            variant=""
            if [[ "$e" == *:* ]]; then variant="${e#*:}"; fi
            json+="{\"layout\":\"${e%%:*}\",\"variant\":\"$variant\"},"
        done
        "$S_BIN" --set keyboard.layouts "${json%,}]"
    fi
    "$S_BIN" --apply --no-reload >/dev/null
    unset INFINITY_HYPRLAND_CONFIG_DIR
fi

# окно настроек должно запускаться из установленного места (а не только из каталога сборки)
if QT_QPA_PLATFORM=offscreen INFINITY_HYPRLAND_CONFIG_DIR="$tmp/smoke-config" timeout 60 "$PREFIX/bin/infinity-settings" --screenshot "$tmp/smoke.png" 0 >/dev/null 2>&1 && [[ -s "$tmp/smoke.png" ]]; then
    echo "Окно настроек запускается."
else
    warn "Окно настроек не запустилось из $PREFIX/bin: SUPER+I его не откроет. Сообщите об этом."
fi

say "Готово ($([[ -n "$PREV_VERSION" ]] && echo "обновлено с $PREV_VERSION" || echo "новая установка"))"
cat <<MSG
  Композитор:  $PREFIX/bin/InfinityHyprland
  Вход:        выберите «Infinity-hyprland» в экране входа (SDDM, GDM и т.д.)
  Конфиг:      $CONF_DIR
  Удалить:     ./install.sh --uninstall
MSG
