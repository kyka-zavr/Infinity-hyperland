# Обновление на новые версии Hyprland

База форка: файл [UPSTREAM](../UPSTREAM) (тег и коммит оригинала).

## Как устроено

- Ветка `upstream` хранит **только чистые импорты** оригинала, один коммит на версию. Наш код там не появляется.
- `scripts/sync-upstream.sh [тег]` делает новый импорт и вливает его в ветку `sync/<тег>` от `main` обычным merge. Общая база слияния: прошлый импорт, поэтому конфликтуют только файлы, которые правили мы.
- Закрепления подпроектов (`scripts/subprojects.pins`) обновляются автоматически по ссылкам оригинала.
- `main` скрипт не меняет. Результат собирается, проверяется руками (`dev/run-nested.sh`) и вливается через pull request.
- `.github/workflows/upstream-sync.yml` раз в сутки проверяет новый стабильный релиз: чистое слияние даёт ветку, проверочную сборку на Arch и pull request; конфликты дают issue со списком файлов. Для автоматического pull request в настройках репозитория (Settings, Actions, General) должно быть включено «Allow GitHub Actions to create and approve pull requests».
- Зависимости (hyprutils, aquamarine, hyprlang и др.) берутся из системы. Новая версия Hyprland может требовать новых версий или новых библиотек: список для Arch в `installer/arch-deps.txt` сверяется с пакетом `hyprland` при каждом обновлении.

## Состояние на 2026-11-01

Пробное слияние **v0.57.0** (оригинал ушёл вперёд на релиз): **12 конфликтующих файлов**, 718 изменённых файлов в оригинале. Автоматически его не влить. Основная причина: в 0.57 переработана обработка клавиш (`src/managers/KeybindManager.cpp` удалён, код переехал в `src/keybinds/`), плюс изменения в рендере и вводе.

Файлы с конфликтами (наши точки встраивания в чужой код):

- `desktop/state/ViewHitTester.{cpp,hpp}`, `desktop/view/Window.cpp`: поиск окна под курсором, видимость окна.
- `layout/LayoutManager.cpp`, `layout/supplementary/DragController.cpp`: сдвиг, перетаскивание.
- `managers/KeybindManager.cpp` (перенесён), `managers/input/InputManager.cpp`: зум колесом, клики и движение для переключателя.
- `output/Monitor.cpp`, `render/ElementRenderer.cpp`, `render/Renderer.cpp`, `render/pass/RendererHintsPassElement.hpp`: отрисовка зума, панель переключателя.
- `helpers/SystemInfo.cpp`: подпись форка в `--version`.

Новые файлы (`src/switcher/`, `layout/algorithm/floating/infinity/`) конфликтовать не могут. Чем меньше мы правим чужих файлов, тем дешевле обновления: при следующей доработке стоит выносить логику в свои файлы и оставлять в чужих только вызовы.

## Как обновиться вручную

```sh
scripts/sync-upstream.sh v0.57.0      # создаст sync/v0.57.0; при конфликтах остановится
git status                            # список конфликтов
# решить, git add, git commit
./install.sh --no-deps                # или cmake --build build, проверить во вложенном окне
git push origin sync/v0.57.0          # и pull request в main
```
