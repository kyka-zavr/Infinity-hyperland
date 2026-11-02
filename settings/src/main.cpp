// Copyright (c) 2026, kyka-zavr.
//
// infinity-settings
//   --apply [--no-reload]       settings.json + theme -> infinity.lua, templates, reload
//   --print                     print the generated Lua instead of writing it
//   --init                      write the default settings.json and the builtin themes when missing
//   --get <path>                print a setting (dotted path, JSON)
//   --set <path> <json>         change a setting (does not apply by itself)
//   --list-themes
//   (no arguments)              the settings window

#include "Applier.hpp"
#include "LuaGenerator.hpp"
#include "Paths.hpp"
#include "SettingsStore.hpp"
#include "Theme.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTextStream>

static QTextStream& out() {
    static QTextStream s(stdout);
    return s;
}

static int runCli(const QStringList& args) {
    SettingsStore store;
    store.load();

    if (args.contains("--list-themes")) {
        Theme::installBuiltins();
        for (const auto& n : Theme::available())
            out() << n << "\n";
        return 0;
    }

    if (args.contains("--init")) {
        QDir().mkpath(Paths::configDir());
        Theme::installBuiltins();
        if (!QFile::exists(Paths::settingsFile()) && !store.save())
            return 1;
        out() << "initialized " << Paths::configDir() << "\n";
        return 0;
    }

    if (const auto I = args.indexOf("--get"); I >= 0 && I + 1 < args.size()) {
        const auto V = store.get(args[I + 1]);
        out() << QString::fromUtf8(QJsonDocument(QJsonArray{V}).toJson(QJsonDocument::Compact)).mid(1).chopped(1) << "\n";
        return V.isUndefined() ? 1 : 0;
    }

    if (const auto I = args.indexOf("--set"); I >= 0 && I + 2 < args.size()) {
        QJsonParseError err;
        const auto      DOC = QJsonDocument::fromJson("[" + args[I + 2].toUtf8() + "]", &err);
        if (err.error != QJsonParseError::NoError || DOC.array().size() != 1) {
            out() << "invalid JSON value: " << args[I + 2] << "\n";
            return 2;
        }
        store.set(args[I + 1], DOC.array().first());
        return store.save() ? 0 : 1;
    }

    if (args.contains("--print")) {
        Theme::installBuiltins();
        out() << LuaGenerator::generate(store, Theme::load(store.get("theme").toString("dark")), QCoreApplication::applicationFilePath());
        return 0;
    }

    if (args.contains("--apply")) {
        const auto R = Applier::apply({.reload = !args.contains("--no-reload")});
        for (const auto& m : R.messages)
            out() << m << "\n";
        return R.ok ? 0 : 1;
    }

    out() << "usage: infinity-settings [--apply [--no-reload] | --print | --init | --get <path> | --set <path> <json> | --list-themes]\n";
    return 2;
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    const auto ARGS = app.arguments().mid(1);
    if (ARGS.isEmpty()) {
        out() << "the settings window is not built yet; use --help\n";
        return 2;
    }

    return runCli(ARGS);
}
