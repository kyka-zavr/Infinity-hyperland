#!/usr/bin/env bash
# Подтягивает новый релиз Hyprland в форк.
#
#   scripts/sync-upstream.sh            последний стабильный релиз (без -rc)
#   scripts/sync-upstream.sh v0.57.0    конкретный тег
#
# Как это устроено:
#   * ветка `upstream` хранит ТОЛЬКО чистые импорты оригинала, один коммит на версию (hyprland/ целиком);
#   * новый импорт вливается в отдельную ветку sync/<тег> от main обычным merge, общей базой служит прошлый импорт,
#     поэтому конфликтуют только файлы, которые мы сами меняли;
#   * main не трогается: результат проверяется сборкой и вливается через pull request.
set -euo pipefail

cd "$(git -C "$(dirname "${BASH_SOURCE[0]}")" rev-parse --show-toplevel)"

UPSTREAM_URL="https://github.com/hyprwm/Hyprland.git"
TAG="${1:-}"

die() { printf 'sync-upstream: %s\n' "$*" >&2; exit 1; }

[[ -z "$(git status --porcelain)" ]] || die "в рабочем дереве есть незакоммиченные изменения"
git rev-parse --verify -q refs/heads/upstream >/dev/null || die "нет ветки upstream (чистый импорт первой версии)"

git remote get-url upstream >/dev/null 2>&1 || git remote add upstream "$UPSTREAM_URL"

if [[ -z "$TAG" ]]; then
    TAG=$(git ls-remote --tags --refs upstream 'v[0-9]*' | awk '{print $2}' | sed 's|refs/tags/||' | grep -v -- '-' | sort -V | tail -1)
    [[ -n "$TAG" ]] || die "не нашёл релизных тегов в оригинале"
fi

CURRENT=$(sed -n 1p UPSTREAM)
[[ "$TAG" != "$CURRENT" ]] || { echo "Уже на $TAG, обновлять нечего."; exit 0; }
[[ "$(printf '%s\n%s\n' "$CURRENT" "$TAG" | sort -V | tail -1)" == "$TAG" ]] || die "$TAG старше текущей базы $CURRENT"
git rev-parse --verify -q "refs/heads/sync/$TAG" >/dev/null && die "ветка sync/$TAG уже существует (удалите её, если нужно начать заново)"

echo "База форка: $CURRENT, новая: $TAG"
git fetch --quiet --depth 1 upstream "refs/tags/$TAG:refs/upstream-tags/$TAG"
COMMIT=$(git rev-parse "refs/upstream-tags/$TAG^{commit}")

# 1. чистый импорт новой версии в ветку upstream (без переключения веток); уже сделанный импорт используется повторно
imp=$(git log refs/heads/upstream --grep="^Import Hyprland $TAG (" -1 --format=%H)
if [[ -z "$imp" ]]; then
    TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
    git archive "$COMMIT" --prefix=hyprland/ | tar -x -C "$TMP/"
    IDX="$TMP.index"
    tree=$(GIT_INDEX_FILE="$IDX" bash -c 'git --work-tree="$1" add -A -f && git write-tree' _ "$TMP")
    rm -f "$IDX"
    imp=$(git commit-tree "$tree" -p refs/heads/upstream -m "Import Hyprland $TAG ($COMMIT) unmodified

Pristine source tree of $UPSTREAM_URL at $TAG.
Submodule contents are not included, see scripts/fetch-subprojects.sh.")
    git update-ref refs/heads/upstream "$imp"
    echo "Импорт: upstream -> ${imp:0:7}"
else
    echo "Импорт $TAG уже есть: ${imp:0:7}"
fi

# 2. слияние в отдельную ветку
git switch -q -c "sync/$TAG" main
if ! git merge --no-ff -m "Merge Hyprland $TAG into the fork" upstream; then
    echo
    echo "КОНФЛИКТЫ при слиянии с $TAG. Файлы:"
    git diff --name-only --diff-filter=U | sed 's/^/  /'
    echo "Решите их, затем: git add <файлы> && git commit, и повторите проверку сборки."
    exit 2
fi

# 3. закрепление версий подпроектов по gitlink'ам оригинала
pins=$(mktemp)
while read -r line; do
    [[ -z "$line" || "$line" == \#* ]] && { echo "$line" >> "$pins"; continue; }
    read -r name url rev <<<"$line"
    new=$(git ls-tree "$COMMIT" "subprojects/$name" | awk '$2=="commit" {print $3}')
    if [[ -z "$new" ]]; then
        echo "Подпроект $name больше не нужен оригиналу (теперь системная зависимость?), убран из subprojects.pins"
        continue
    fi
    echo "$name $url $new" >> "$pins"
    [[ "$new" == "$rev" ]] || echo "Подпроект $name: ${rev:0:7} -> ${new:0:7}"
done < scripts/subprojects.pins
mv "$pins" scripts/subprojects.pins

printf '%s\n%s\n' "$TAG" "$COMMIT" > UPSTREAM
git add UPSTREAM scripts/subprojects.pins
git commit -q -m "Fork base is now Hyprland $TAG"

echo
echo "Готово: ветка sync/$TAG. Дальше: собрать и проверить, затем открыть pull request в main."
echo "Зависимости оригинала могли измениться: сверьте список DEPS в install.sh с depends пакета hyprland."
