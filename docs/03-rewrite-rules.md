# Правила работы с кодом Hyprland

Проект — форк Hyprland, поэтому действует режим B: код Hyprland берём и меняем, а лицензия BSD 3-Clause на него распространяется.

## Обязательно

1. Оставить в каждом заимствованном файле оригинальный copyright. В репозитории оставить файл LICENSE оригинала и не менять его текст.
2. В бинарных сборках поставлять текст лицензии (файл лицензий, экран «О программе», пакет).
3. Не использовать имена vaxerski и Hyprland для рекламы нашего форка без письменного разрешения. Говорить «форк Hyprland» как факт можно, но не как рекламу от имени автора.
4. Наши изменения помечать своим copyright рядом с оригинальным. Само уведомление автора не удалять.

## Заголовок для изменённых и новых файлов

Изменённый файл Hyprland:

```
// Copyright (c) 2022-2026, vaxerski. BSD 3-Clause License. (original Hyprland)
// Modifications Copyright (c) <год>, kyka-zavr.
```

Новый файл, написанный нами:

```
// Copyright (c) <год>, kyka-zavr.
```

Владелец форка: kyka-zavr.

## Название

Форк называем иначе, чем оригинал, и не используем логотип Hyprland. Рабочее название: Infinity-hyperland.

## Журнал расхождений с upstream

| Дата | Что изменили | Файлы |
|---|---|---|
| 2026-10-22 | База: Hyprland v0.56.2, коммит `efb5099`. | |
| 2026-10-22 | Внешнее переименование: бинарник `InfinityHyprland`, строка версии, copyright kyka-zavr. | `CMakeLists.txt`, `src/helpers/SystemInfo.cpp`, `LICENSE` |
| 2026-10-22 | Infinity, шаги 1-2: плавающий алгоритм `CInfinityFloatingAlgorithm` (новые файлы), переключение режима и сдвиг через `layoutmsg infinity ...` на все мониторы, сдвиг мышью при зажатых модификаторах (опция `infinity:pan_mods`), новые окна в Infinity свободные. | новые: `src/layout/algorithm/floating/infinity/*`; изменены: `src/layout/algorithm/Algorithm.{hpp,cpp}`, `src/layout/LayoutManager.{hpp,cpp}`, `src/config/values/ConfigValues.cpp`, `src/layout/algorithm/floating/default/DefaultFloatingAlgorithm.hpp` (добавлен `#pragma once`) |
| 2026-10-22 | Infinity, шаг 3 (рендер зума): окна рабочего стола в Infinity рисуются с масштабом через модификатор рендера; элемент подсказок рендера отключает упрощение пасса (иначе теряются окна), для поверхностей повреждение сопоставляется в масштабированном пространстве. Сообщение `infinity zoom <множитель> [<x> <y>]`. | `src/render/Renderer.cpp`, `src/render/ElementRenderer.cpp`, `src/render/pass/RendererHintsPassElement.{hpp,cpp}`, `src/layout/algorithm/Algorithm.{hpp,cpp}`, `src/layout/algorithm/floating/infinity/*` |
| 2026-10-22 | Infinity, шаг 3 (ввод и повреждение при зуме): окна ищутся в координатах стола, перетаскивание и ресайз делятся на зум, при зуме повреждается весь монитор, `windowAt` принимает монитор явно, сообщения `zoom ... cursor` и `probe`, зум без Infinity пропускает событие приложению. | `src/managers/input/InputManager.{hpp,cpp}`, `src/managers/KeybindManager.cpp`, `src/layout/supplementary/DragController.cpp`, `src/desktop/state/ViewHitTester.{hpp,cpp}`, `src/output/Monitor.cpp`, `src/layout/LayoutManager.{hpp,cpp}`, `src/config/shared/actions/ConfigActions.cpp` |
| 2026-10-22 | Infinity, шаг 4: `alttab` (история окон, центрирование в Infinity, переключение стола в тайловом режиме); перенос курсора на окно учитывает зум. | `src/config/shared/actions/ConfigActions.cpp`, `src/layout/LayoutManager.{hpp,cpp}`, `src/desktop/view/Window.cpp` |
| 2026-10-22 | Исправления Infinity: зум колесом встроен (без задержки бинда), сдвиг стола сырым движением мыши (курсор не упирается в край, 1:1 при зуме), `visibleOnMonitor` учитывает отдалённый вид. Опции `infinity:zoom_mods`, `infinity:zoom_step`. | `src/managers/KeybindManager.cpp`, `src/managers/input/InputManager.cpp`, `src/layout/LayoutManager.{hpp,cpp}`, `src/desktop/view/Window.cpp`, `src/config/values/ConfigValues.cpp` |

## Смежные библиотеки

Перед использованием кода или зависимости из других проектов hyprwm сверяем лицензию в таблице [02-licensing.md](02-licensing.md).
