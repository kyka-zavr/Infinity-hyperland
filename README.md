# Infinity-hyperland

> **Это форк [Hyprland](https://github.com/hyprwm/Hyprland), а не оригинальный проект.**
> Hyprland написан vaxerski и контрибьюторами hyprwm (BSD 3-Clause), вся основа композитора принадлежит им.
> С авторами Hyprland этот форк не связан и ими не одобрен. База: **v0.57.0** (`0aa6745`).
> Здесь только наши изменения поверх, они перечислены ниже и видны в истории git.
>
> *English: a fork of the Hyprland Wayland compositor that adds a native infinite canvas mode and an Alt-Tab window switcher with live previews. Not affiliated with the Hyprland authors.*

Wayland-композитор с **бесконечным столом**: окна лежат на одной бесконечной плоскости, экран — окно-обзор, которое двигается и масштабируется по ней. Идея взята у VXWM (Versatile X Window Manager, форк dwm; [зеркало на GitHub](https://github.com/wh1tepearll/vxwm)), реализована внутри ядра Hyprland, а не плагином. Обычный тайловый режим остаётся, режимы переключаются на лету.

## Что добавлено относительно Hyprland

- **Режим Infinity.** Свободные окна на бесконечной плоскости. Переключение режима не теряет места окон: выйдя из режима, они возвращаются на свои тайловые места.
- **Сдвиг и зум стола** (от 0.1 до 4), общие для всех мониторов: плоскость цельная, окно можно протащить с экрана на экран.
- **Переключатель окон `ALT+TAB`** с живыми превью, иконками и названиями приложений. Показывает окна всех рабочих столов на каждом мониторе, работает в обоих режимах и центрирует выбранное окно на столе.

## Управление

Бинды из `hyprland/example/hyprland.lua` (главный модификатор `SUPER`):

| Действие | Клавиши |
|---|---|
| Включить / выключить Infinity | `SUPER+SHIFT+SPACE` |
| Двигать окно / менять размер | `SUPER+ЛКМ` / `SUPER+ПКМ` (перетаскивание) |
| Двигать весь стол | движение мыши при зажатых `SUPER+ALT` (или `SUPER+CTRL+стрелки`) |
| Зум | `CTRL+колесо` (или `SUPER+CTRL+=` и `SUPER+CTRL+-`) |
| Переключатель окон | `ALT+TAB`, назад `ALT+SHIFT+TAB`; стрелки, мышь, `ENTER`, `ESC` |

Опции: `infinity:pan_mods`, `infinity:zoom_mods`, `infinity:zoom_step`. Команды: `hyprctl dispatch 'hl.dsp.layout("infinity toggle|on|off|pan <dx> <dy>|zoom <k>")'`, `alttab`, `alttab prev`.

## Установка (Arch и производные)

```sh
git clone https://github.com/kyka-zavr/Infinity-hyperland.git
cd Infinity-hyperland
./install.sh
```

Установщик проверяет и ставит зависимости, собирает композитор и добавляет пункт **«Infinity-hyprland»** в экран входа (SDDM, GDM, greetd и др.). Что важно:

- Ставится в отдельный каталог `/opt/infinity-hyprland`, установленный Hyprland (`/usr/bin/Hyprland`, `hyprctl`) **не затрагивается**. Оба можно держать в системе и выбирать при входе.
- Конфиг отдельный: `~/.config/infinity-hyprland/`. Установщик спрашивает: **скопировать нужные файлы из `~/.config/hypr`** (оригинал не меняется, дальше две независимые копии) или **создать чистый** свой. Существующий конфиг никогда не перезаписывается.
- Удаление: `./install.sh --uninstall` (конфиги остаются).

Параметры: `--config copy|fresh|skip`, `--prefix DIR`, `--yes`, `--no-deps`. Установка проверена на тестовом префиксе и во вложенном запуске; вход через экран логина на реальной системе пока не проверен.

## Сборка вручную и запуск для разработки

Зависимости те же, что у Hyprland, список для Arch: [installer/arch-deps.txt](installer/arch-deps.txt).

```sh
scripts/fetch-subprojects.sh                       # подпроект udis86 (hyprland-protocols с 0.57 системный пакет)
cmake -S hyprland -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j3                            # на слабой машине следите за памятью, -j$(nproc) может её исчерпать
dev/run-nested.sh                                  # запуск вложенным окном внутри текущей Wayland-сессии
```

Бинарник: `build/InfinityHyprland`. Во вложенном окне главный модификатор `ALT`, переключатель окон на `CTRL+TAB` (конфиг `dev/hyprland.lua`). Настоящая сессия (DRM из TTY) пока не проверена.

## Обновления

База форка записана в [UPSTREAM](UPSTREAM). Новые релизы Hyprland подтягивает `scripts/sync-upstream.sh` (и ежедневная проверка на GitHub): подробности и текущее состояние в [docs/07-updating.md](docs/07-updating.md).

## Состояние

Рабочие: режим Infinity, сдвиг, зум, несколько мониторов, переключатель окон. Проверено только во вложенном окне. Подробности, известные недочёты и план: [docs/04-roadmap.md](docs/04-roadmap.md).

## Документы

[docs/](docs/): обзор, лицензии, правила работы с кодом, план, дизайн бесконечного стола, карта кода.

## Лицензия

BSD 3-Clause, как у Hyprland: [LICENSE](LICENSE). Copyright (c) 2022-2026 vaxerski, (c) 2026 kyka-zavr (изменения). Сторонний код и лицензии зависимостей: [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md), [docs/02-licensing.md](docs/02-licensing.md).
