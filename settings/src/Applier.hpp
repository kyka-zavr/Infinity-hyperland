// Copyright (c) 2026, kyka-zavr.
#pragma once

#include <QString>
#include <QStringList>

namespace Applier {
    struct Options {
        bool reload = true;   // run `hyprctl reload` afterwards
    };

    struct Result {
        bool        ok = false;
        QStringList messages;
    };

    // settings.json (+ the theme) -> infinity.lua, templates, post command, reload
    Result apply(const Options& options);
}
