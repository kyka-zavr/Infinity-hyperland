// Copyright (c) 2026, kyka-zavr.
#include "Applier.hpp"

#include "LuaGenerator.hpp"
#include "Paths.hpp"
#include "SettingsStore.hpp"
#include "Templates.hpp"
#include "Theme.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QSaveFile>
#include <QStandardPaths>

static QString findHyprctl() {
    if (const auto E = qEnvironmentVariable("INFINITY_HYPRCTL"); !E.isEmpty())
        return E;

    const auto NEXT = QCoreApplication::applicationDirPath() + "/hyprctl";
    if (QFile::exists(NEXT))
        return NEXT;

    return QStandardPaths::findExecutable("hyprctl");
}

Applier::Result Applier::apply(const Options& options) {
    Result r;

    SettingsStore store;
    if (!store.load())
        r.messages << "settings.json missing or invalid: using the defaults";

    QDir().mkpath(Paths::configDir());
    Theme::installBuiltins();

    const auto THEME = Theme::load(store.get("theme").toString("dark"));
    const auto LUA   = LuaGenerator::generate(store, THEME, QCoreApplication::applicationFilePath());

    QSaveFile f(Paths::generatedLua());
    if (!f.open(QIODevice::WriteOnly) || f.write(LUA.toUtf8()) < 0 || !f.commit()) {
        r.messages << "cannot write " + Paths::generatedLua();
        return r;
    }
    r.messages << "wrote " + Paths::generatedLua();

    QStringList errors;
    for (const auto& t : Templates::renderAll(THEME, errors))
        r.messages << "template -> " + t;
    for (const auto& e : errors)
        r.messages << "template error: " + e;

    if (const auto CMD = store.get("export.post_command").toString(); !CMD.isEmpty()) {
        QProcess p;
        p.start("/bin/sh", {"-c", CMD});
        p.waitForFinished(15000);
        r.messages << QString("post command exited with %1").arg(p.exitCode());
    }

    if (options.reload) {
        const auto HYPRCTL = findHyprctl();
        if (HYPRCTL.isEmpty())
            r.messages << "hyprctl not found: the compositor was not reloaded";
        else {
            QProcess p;
            p.start(HYPRCTL, {"reload"});
            p.waitForFinished(15000);
            r.messages << (p.exitCode() == 0 ? "compositor reloaded" : "hyprctl reload failed (no running session?)");
        }
    }

    r.ok = true;
    return r;
}
