// Copyright (c) 2026, kyka-zavr.
#pragma once

#include "Theme.hpp"

#include <QString>
#include <QStringList>

// Exports the palette to other programs. Every templates/*.tmpl file starts with a line
//     @target: ~/.config/kitty/infinity-colors.conf
// and the rest is the file content with placeholders:
//     {{accent}} -> #5c8fff   {{accent.nohash}} -> 5c8fff   {{accent.rgb}} -> 92,143,255   {{accent.rgba}} -> 92,143,255,1.0
namespace Templates {
    QString renderText(const QString& tmpl, const Theme& theme);
    // renders every template, returns the written targets and fills `errors`
    QStringList renderAll(const Theme& theme, QStringList& errors);
}
