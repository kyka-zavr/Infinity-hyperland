// Copyright (c) 2026, kyka-zavr.
#pragma once

#include <QString>

// Where the files of Infinity-hyprland live. The directory is the one the session wrapper points the compositor to.
namespace Paths {
    QString configDir();     // ~/.config/infinity-hyprland (INFINITY_HYPRLAND_CONFIG_DIR, or the directory of HYPRLAND_CONFIG)
    QString settingsFile();  // settings.json
    QString themesDir();     // themes/
    QString templatesDir();  // templates/
    QString generatedLua();  // infinity.lua (generated)
    QString expandHome(const QString& path);
}
