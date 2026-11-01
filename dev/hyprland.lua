-- Конфиг для вложенного запуска (dev/run-nested.sh).
-- Главный модификатор ALT, а переключатель окон на CTRL+TAB: SUPER и ALT+TAB перехватывает внешняя сессия.
-- Бинды для настоящей сессии: hyprland/example/hyprland.lua (раздел INFINITY).

local mod = "ALT"
local terminal = "kitty"

hl.monitor({
    output   = "",
    mode     = "1280x800",
    position = "auto",
    scale    = 1,
})

hl.bind(mod .. " + Q", hl.dsp.exec_cmd(terminal))
hl.bind(mod .. " + C", hl.dsp.window.close())
hl.bind(mod .. " + M", hl.dsp.exec_cmd("hyprctl dispatch 'hl.dsp.exit()'"))
hl.bind(mod .. " + V", hl.dsp.window.float({ action = "toggle" }))

hl.bind(mod .. " + left",  hl.dsp.focus({ direction = "left" }))
hl.bind(mod .. " + right", hl.dsp.focus({ direction = "right" }))
hl.bind(mod .. " + up",    hl.dsp.focus({ direction = "up" }))
hl.bind(mod .. " + down",  hl.dsp.focus({ direction = "down" }))

-- Infinity: стол двигается движением мыши при зажатых CTRL+ALT (в настоящей сессии по умолчанию SUPER+ALT), зум: CTRL + колесо
hl.config({ infinity = { pan_mods = "CTRL ALT", zoom_mods = "CTRL" } })
hl.bind(mod .. " + SHIFT + SPACE", hl.dsp.layout("infinity toggle"))
hl.bind("CTRL + Tab",          hl.dsp.layout("alttab"))
hl.bind("CTRL + SHIFT + Tab", hl.dsp.layout("alttab prev"))
hl.bind(mod .. " + CTRL + left",   hl.dsp.layout("infinity pan 150 0"))
hl.bind(mod .. " + CTRL + right",  hl.dsp.layout("infinity pan -150 0"))
hl.bind(mod .. " + CTRL + up",     hl.dsp.layout("infinity pan 0 150"))
hl.bind(mod .. " + CTRL + down",   hl.dsp.layout("infinity pan 0 -150"))
hl.bind(mod .. " + CTRL + equal",  hl.dsp.layout("infinity zoom 1.25"))
hl.bind(mod .. " + CTRL + minus",  hl.dsp.layout("infinity zoom 0.8"))

hl.bind(mod .. " + mouse:272", hl.dsp.window.drag(),   { mouse = true })
hl.bind(mod .. " + mouse:273", hl.dsp.window.resize(), { mouse = true })
