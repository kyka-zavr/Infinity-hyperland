-- >>> infinity-hyprland: добавлено установщиком. Можно менять и удалять.
-- Режим Infinity на всех мониторах. Выход из режима возвращает окна на тайловые места.
hl.bind("SUPER + SHIFT + SPACE", hl.dsp.layout("infinity toggle"))

-- Переключатель окон: зажать ALT, TAB листает, отпустить ALT выбирает (SHIFT+TAB назад).
hl.bind("ALT + Tab",         hl.dsp.layout("alttab"))
hl.bind("ALT + SHIFT + Tab", hl.dsp.layout("alttab prev"))

-- Сдвиг и зум стола с клавиатуры (мышью: движение при зажатых SUPER+ALT, зум CTRL+колесо).
hl.bind("SUPER + CTRL + left",  hl.dsp.layout("infinity pan 150 0"),  { repeating = true })
hl.bind("SUPER + CTRL + right", hl.dsp.layout("infinity pan -150 0"), { repeating = true })
hl.bind("SUPER + CTRL + up",    hl.dsp.layout("infinity pan 0 150"),  { repeating = true })
hl.bind("SUPER + CTRL + down",  hl.dsp.layout("infinity pan 0 -150"), { repeating = true })
hl.bind("SUPER + CTRL + equal", hl.dsp.layout("infinity zoom 1.25"),  { repeating = true })
hl.bind("SUPER + CTRL + minus", hl.dsp.layout("infinity zoom 0.8"),   { repeating = true })
-- <<< infinity-hyprland
