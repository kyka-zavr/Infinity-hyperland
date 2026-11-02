-- >>> infinity-hyprland: added by the installer. The binds, keyboard layouts and colors of the fork are generated into infinity.lua
-- (next to this file) from the settings window (SUPER+I). Remove this block to stop using them.
local infinityDir = (os.getenv("HYPRLAND_CONFIG") or ""):match("^(.*)/[^/]*$")
local infinityFile = infinityDir and io.open(infinityDir .. "/infinity.lua")
if infinityFile then
    infinityFile:close()
    dofile(infinityDir .. "/infinity.lua")
end
-- <<< infinity-hyprland
