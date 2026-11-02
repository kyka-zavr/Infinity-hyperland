// Copyright (c) 2026, kyka-zavr.
#pragma once

#include "SettingsStore.hpp"
#include "Theme.hpp"

#include <QList>
#include <QString>

// The binds that belong to this fork. Each has a default key and can be rebound or disabled in the settings.
struct ManagedBind {
    QString id;
    QString label;
    QString keys;      // default, e.g. "SUPER + SHIFT + SPACE"
    QString action;    // Lua expression for hl.bind; %SETTINGS% is replaced by the settings program path
    bool    repeating = false;
};

namespace LuaGenerator {
    const QList<ManagedBind>& managedBinds();

    // the keys a managed bind uses now: the user's choice or the default ("" = no key)
    QString effectiveKeys(const SettingsStore& store, const ManagedBind& bind);

    // the whole generated infinity.lua
    QString generate(const SettingsStore& store, const Theme& theme, const QString& settingsBinary);

    QString quote(const QString& s); // a Lua string literal
}
