# Обновление на новые версии Hyprland

База форка: файл [UPSTREAM](../UPSTREAM) (тег и коммит оригинала).

## Как устроено

- Ветка `upstream` хранит **только чистые импорты** оригинала, один коммит на версию. Наш код там не появляется.
- `scripts/sync-upstream.sh [тег]` делает новый импорт и вливает его в ветку `sync/<тег>` от `main` обычным merge. Общая база слияния: прошлый импорт, поэтому конфликтуют только файлы, которые правили мы.
- Закрепления подпроектов (`scripts/subprojects.pins`) обновляются автоматически по ссылкам оригинала.
- `main` скрипт не меняет. Результат собирается, проверяется руками (`dev/run-nested.sh`) и вливается через pull request.
- `.github/workflows/upstream-sync.yml` раз в сутки проверяет новый стабильный релиз: чистое слияние даёт ветку, проверочную сборку на Arch и pull request; конфликты дают issue со списком файлов. Для автоматического pull request в настройках репозитория (Settings, Actions, General) должно быть включено «Allow GitHub Actions to create and approve pull requests».
- Зависимости (hyprutils, aquamarine, hyprlang и др.) берутся из системы. Новая версия Hyprland может требовать новых версий или новых библиотек: список для Arch в `installer/arch-deps.txt` сверяется с пакетом `hyprland` при каждом обновлении.

## Состояние

База форка: **v0.57.0** (перенос сделан 2026-11-01 с v0.56.2).

Что пришлось перенести (для понимания, сколько стоит такое обновление): в 0.57 переработаны рендер (контекст `Render::CRenderContext`, сцена монитора), ввод и клавиши (`src/keybinds/`, `KeybindManager.cpp` удалён), окно (`desktop/view/Window.cpp` разбит на `desktop/view/window/*`), рабочее место (`space()` вместо `m_space`), требования к зависимостям (`hyprwire`, `hyprland-protocols` как системный пакет вместо подпроекта). Конфликтовало 12 файлов, остальное правилось по ошибкам сборки. Режим Infinity, зум, переключатель окон и установщик работают на 0.57.0, проверено во вложенном окне с двумя мониторами.

Наши точки встраивания в чужой код (то, что чаще всего конфликтует при обновлении):

- `desktop/state/ViewHitTester.*`, `desktop/view/window/Window.cpp`, `desktop/view/window/WindowPresentation.cpp`: поиск окна под курсором, видимость окна, перенос курсора.
- `layout/LayoutManager.cpp`, `layout/supplementary/DragController.cpp`: сдвиг, перетаскивание.
- `keybinds/Manager.cpp`, `managers/input/InputManager.cpp`, `config/shared/actions/ConfigActions.cpp`: зум колесом, клики и движение для переключателя, клавиши переключателя.
- `output/Monitor.cpp`, `render/ElementRenderer.cpp`, `render/Renderer.cpp`, `render/pass/RendererHintsPassElement.*`: отрисовка зума, панель переключателя.
- `helpers/SystemInfo.cpp`: подпись форка в `--version`; `CMakeLists.txt`: без ссылки `hyprland` при установке.

Новые файлы (`src/switcher/`, `layout/algorithm/floating/infinity/`) конфликтовать не могут. Чем меньше мы правим чужих файлов, тем дешевле обновления: при доработках стоит выносить логику в свои файлы и оставлять в чужих только вызовы.

## Как обновиться вручную

```sh
scripts/sync-upstream.sh v0.57.0      # создаст sync/v0.57.0; при конфликтах остановится
git status                            # список конфликтов
# решить, git add, git commit
./install.sh --no-deps                # или cmake --build build, проверить во вложенном окне
git push origin sync/v0.57.0          # и pull request в main
```
