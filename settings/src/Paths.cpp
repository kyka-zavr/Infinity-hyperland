// Copyright (c) 2026, kyka-zavr.
#include "Paths.hpp"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

QString Paths::configDir() {
    if (const auto E = qEnvironmentVariable("INFINITY_HYPRLAND_CONFIG_DIR"); !E.isEmpty())
        return E;

    if (const auto C = qEnvironmentVariable("HYPRLAND_CONFIG"); !C.isEmpty())
        return QFileInfo(C).absolutePath();

    const auto X = qEnvironmentVariable("XDG_CONFIG_HOME");
    return (X.isEmpty() ? QDir::homePath() + "/.config" : X) + "/infinity-hyprland";
}

QString Paths::settingsFile() {
    return configDir() + "/settings.json";
}

QString Paths::themesDir() {
    return configDir() + "/themes";
}

QString Paths::templatesDir() {
    return configDir() + "/templates";
}

QString Paths::generatedLua() {
    return configDir() + "/infinity.lua";
}

QString Paths::expandHome(const QString& path) {
    if (path == "~" || path.startsWith("~/"))
        return QDir::homePath() + path.mid(1);
    return path;
}
