// Copyright (c) 2026, kyka-zavr.
#pragma once

#include <QMap>
#include <QString>
#include <QStringList>

// A palette: named colors, all stored as "rrggbbaa" (lowercase hex, no '#').
struct Theme {
    QString               name;
    QMap<QString, QString> colors;

    QString               color(const QString& key) const;            // "rrggbbaa", falls back to the dark theme
    QString               hex(const QString& key) const;              // "#rrggbb"
    QString               luaColor(const QString& key) const;         // "rgba(rrggbbaa)" for hl.config
    static QString        normalize(const QString& text);             // "#rgb", "#rrggbb", "#rrggbbaa" -> "rrggbbaa" ("" when invalid)

    static QStringList    colorKeys();                                // the palette keys, in display order
    static QStringList    builtinNames();
    static Theme          builtin(const QString& name);
    static Theme          fromJson(const QByteArray& json);
    QByteArray            toJson() const;

    // loads themes/<name>.json, or the builtin theme of that name, or dark
    static Theme          load(const QString& name);
    static void           installBuiltins();                         // writes the builtin themes into the themes directory when missing
    static QStringList    available();
};
