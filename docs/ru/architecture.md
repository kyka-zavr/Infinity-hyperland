# Устройство

[English](../en/architecture.md) | [Русский](architecture.md)

Как устроен форк и где он касается Hyprland. Пути относительно `hyprland/src/`, если не сказано иное. История изменений относительно оригинала: `git diff upstream main -- hyprland/`.

## Принципы

- Форк, а не переписывание: код Hyprland берётся и меняется (BSD 3-Clause), новая логика лежит в **новых файлах**, а в существующие файлы Hyprland добавляются только небольшие хуки. Чем меньше чужих файлов мы трогаем, тем дешевле обновления ([updating.md](updating.md)).
- C++, как и весь Hyprland. Формат конфига, IPC, сокеты и переменные `HYPRLAND_*` сохранены для совместимости; отличается только имя бинарника (`InfinityHyprland`).
- Платформа: Linux / Wayland, те же бэкенды, что у Hyprland (aquamarine: DRM/KMS и вложенное окно).

## Режим Infinity

В Hyprland раскладки уже сделаны подключаемыми: у рабочего стола есть тайловый и плавающий алгоритмы (`layout/algorithm/`). Infinity — плавающий алгоритм `CInfinityFloatingAlgorithm` (`layout/algorithm/floating/infinity/`), производный от стандартного.

- **Мир и вид.** Алгоритм хранит для каждой цели *мировую рамку*. На экране `screen = world - view` в координатах раскладки. Сдвиг меняет вид и заново расставляет все цели, поэтому окно сохраняет свою мировую позицию.
- **Одна плоскость на всё.** Вид (смещение и зум) — одно общее состояние: каждый стол в режиме Infinity на любом мониторе, видимый или скрытый, показывает одну и ту же часть плоскости. Экземпляры алгоритма регистрируются, сдвиг или зум расставляет заново все. Мировые рамки окон и вид сохраняются при выходе из режима (`storeCanvasBoxes`), поэтому повторное включение возвращает каждое окно на его место на плоскости и восстанавливает вид; стол, который ни разу не был в режиме, начинает с чистой плоскости.
- **Зум не часть раскладки.** Окна сохраняют реальный размер. Рендер масштабирует плоскость вокруг начала всей раскладки (не каждого монитора, иначе соседние мониторы показывали бы пересекающееся содержимое): `screen' = screen * zoom`. В физических пикселях монитора это масштаб на `z` и сдвиг на `monitorPosition * (z - 1) * monitorScale` (`render/Renderer.cpp`). Поверхности layer-shell и обои не масштабируются.
- **Ввод следует за зумом.** `CInputManager::planeCoords` переводит точку экрана в точку плоскости (`screen / z`); окна ищутся в координатах плоскости (`desktop/state/ViewHitTester.*`, с явно заданным монитором, потому что точка при зуме может лежать вне всех мониторов), слои и всё остальное в экранных. Перетаскивание и ресайз (`layout/supplementary/DragController.cpp`) переводят указатель через плоскость; монитор под перетаскиваемым окном определяется по его экранному положению. `CWindow::visibleOnMonitor` использует область плоскости `[pos/z, (pos+size)/z]`.
- **Повреждение (damage).** Оно считается в немасштабированных координатах, поэтому при зуме повреждается весь монитор (`output/Monitor.cpp`), а повреждение элементов сопоставляется с масштабированной рамкой (`render/ElementRenderer.cpp`).
- **Переключение режима** — `CAlgorithm::setInfinity` (`layout/algorithm/Algorithm.cpp`): плавающий алгоритм заменяется без сдвига уже расставленных окон; тайловые окна принимаются как плавающие по их текущей геометрии; при выходе окна, которые раньше не были плавающими, снова тайлятся по положению центра. `CLayoutManager::infinityMsg` применяет изменение к активному столу каждого монитора.
- **Окна-оверлеи.** App id из `infinity:overlay_classes` (окно настроек) не часть плоскости: они закреплены (`WINDOW_STATE_PINNED`), поэтому рендер рисует их после сброса зума (в обычном масштабе, поверх окон), и они держатся по центру своего монитора. `CInputManager::planeCoords` считает точку над таким окном экранной и не делит её на зум, а перетаскивание для них запрещено.
- **Сдвиг мышью:** `CLayoutManager::panWithMouse` забирает сырое движение указателя, пока зажаты модификаторы сдвига (относительные устройства замораживают курсор, абсолютные следуют). **Зум колесом** обрабатывается в `keybinds/Manager.cpp::onAxisEvent`.
- Опции объявлены в `config/values/ConfigValues.cpp` (`infinity:*`).

## Переключатель окон

Новый модуль `switcher/`:

- `WindowSwitcher.{hpp,cpp}`: состояние и отрисовка. Список строится из истории окон (все рабочие столы, недавние первыми). Панель рисуется каждый кадр на том мониторе, где находится указатель (на нём переключатель и открыли), обычными элементами рендер-прохода (`CRectPassElement`, `CTexPassElement`); превью — текстуры поверхностей самих окон, поэтому они живые и работают для скрытых рабочих столов. Текст рендерится через `renderText` и кэшируется по размеру шрифта. Открытие, закрытие и выбор анимированы (появление с прозрачностью и лёгким масштабом, акцент скользит между плитками). При подтверждении выбранное окно фокусируется явно, поэтому окно с другого рабочего стола показывается и поднимается.
- `AppInfo.{hpp,cpp}`: класс окна в название приложения и иконку. Разбирает `.desktop`-файлы (`Name`, локализованные `Name[...]`, `Icon`, `StartupWMClass`, `Exec`; скрытые записи и `NoDisplay` не подходят по `Exec`) и ищет иконки в теме GTK, `hicolor`, Papirus, breeze, Adwaita и pixmaps; PNG и SVG загружаются через `Hyprgraphics::CImage`.
- Точки входа в Hyprland: клавиши (`keybinds/Manager.cpp::onKeyEvent`: подтверждает при отпускании модификатора, забирает Esc, Enter и стрелки, пока открыт), кнопки мыши (`managers/input/InputManager.cpp::processMouseDownNormal`), движение указателя (`mouseMoveUnified`), отрисовка (`render/Renderer.cpp`) и сообщение раскладки `alttab` (`config/shared/actions/ConfigActions.cpp`).

## Политика клавиатуры

`keyboard/KeyboardPolicy.*` стоит над `IKeyboard::updateModifiers`, единственным местом, через которое проходят все смены раскладки и замков. Он помнит раскладку и состояние Num Lock для каждого окна (`infinity:kb_layout_mode`, `kb_default_layout`, `num_mode`), применяет их при смене фокуса (событие `window.active`), а в режиме «always» возвращает Num Lock, когда его выключили. Точки входа: события клавиатуры `modifiers` и `keymap` в `managers/input/InputManager.cpp`, сообщение раскладки `kblayout` в `config/shared/actions/ConfigActions.cpp`.

## Настройки, палитра и бинды

`settings/` — отдельная программа на Qt6 `infinity-settings` (см. [design-settings.md](design-settings.md)): `SettingsStore` (settings.json), `Theme` (палитры), `LuaGenerator` (настройки и тема в `infinity.lua`), `Templates` (экспорт палитры), `Applier` (`--apply`), `Backend` (мост для QML) и страницы QML. Композитор о ней ничего не знает, кроме опций `infinity:*`, которые задаёт сгенерированный Lua (`color_*`, клавиатура, зум), и одного расширения: `hl.rebind(старые, новые)` (`config/lua/bindings/LuaBindingsToplevel.cpp`) переносит бинд на другие клавиши с сохранением действия, так бинды из ваших собственных файлов редактируются без переписывания этих файлов.

## Установщик и сессия

`install.sh` и `installer/` (подробности: [installation.md](installation.md)):

- всё ставится в собственный префикс (`/opt/infinity-hyprland`), чтобы не трогать установленный Hyprland; штатное правило `install`, создававшее ссылку `hyprland` рядом с бинарником, убрано из `CMakeLists.txt`;
- `infinity-hyprland-session` — пункт входа: выставляет `XDG_CURRENT_DESKTOP=Infinity-hyprland:Hyprland`, через `HYPRLAND_CONFIG` указывает композитору `~/.config/infinity-hyprland/hyprland.lua` и запускает его через `start-hyprland --path` (перезапуск в безопасном режиме после сбоя);
- `infinity-hyprland-config` создаёт или копирует конфиг и никогда ничего не перезаписывает.

## Где мы трогаем чужие файлы

| Область | Файлы |
|---|---|
| Поиск окна, видимость, перенос курсора | `desktop/state/ViewHitTester.*`, `desktop/view/window/Window.cpp`, `desktop/view/window/WindowPresentation.cpp` |
| Сдвиг, перетаскивание | `layout/LayoutManager.*`, `layout/supplementary/DragController.cpp`, `layout/algorithm/Algorithm.*` |
| Клавиши, колесо, мышь | `keybinds/Manager.cpp`, `managers/input/InputManager.*`, `config/shared/actions/ConfigActions.cpp` |
| Рендер | `output/Monitor.cpp`, `render/Renderer.cpp`, `render/ElementRenderer.cpp`, `render/pass/RendererHintsPassElement.*` |
| Опции, строка версии | `config/values/ConfigValues.cpp`, `helpers/SystemInfo.cpp`, `CMakeLists.txt` |
| Бинды из Lua | `config/lua/bindings/LuaBindingsToplevel.cpp` (`hl.rebind`) |

Новые файлы: `layout/algorithm/floating/infinity/*`, `switcher/*`, `keyboard/*`; вне композитора: `settings/`.

## Работа с кодом

- Каждый новый файл начинается с `// Copyright (c) <год>, kyka-zavr.` В файлах, взятых из Hyprland, оригинальный copyright остаётся, а наш добавляется рядом, но не вместо него.
- По возможности добавляйте код в новые файлы и вызывайте его из однострочного хука в чужом файле.
- Собирайте с лимитом памяти (см. [installation.md](installation.md)); конфиг проверяйте `InfinityHyprland --verify-config -c <файл>`; тестируйте во вложенной сессии (`dev/run-nested.sh`, два монитора через `hyprctl output create wayland`).
