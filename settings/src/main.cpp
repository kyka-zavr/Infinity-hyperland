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

#include "Backend.hpp"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>
#include <QtQml>
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

static int runGui(int argc, char** argv, const QString& screenshot, int page) {
    QGuiApplication app(argc, argv);
    QGuiApplication::setDesktopFileName("infinity-settings"); // the window class used by window rules
    QGuiApplication::setApplicationName("infinity-settings");
    QQuickStyle::setStyle("Basic");

    Backend backend;
    qmlRegisterSingletonInstance("Infinity", 1, 0, "Backend", &backend);

    QQmlApplicationEngine engine;
    engine.loadFromModule("Infinity", "Main");
    if (engine.rootObjects().isEmpty())
        return 1;

    auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    if (!screenshot.isEmpty() && window) {
        window->setProperty("page", page);
        QTimer::singleShot(1200, [window, screenshot, &app] {
            window->grabWindow().save(screenshot);
            app.quit();
        });
    }

    return app.exec();
}

int main(int argc, char** argv) {
    QStringList args;
    for (int i = 1; i < argc; ++i)
        args << QString::fromLocal8Bit(argv[i]);

    // the window: no arguments, or --screenshot <file> [page] for a headless render (QT_QPA_PLATFORM=offscreen)
    if (args.isEmpty())
        return runGui(argc, argv, {}, 0);
    if (args[0] == "--screenshot" && args.size() >= 2)
        return runGui(argc, argv, args[1], args.size() >= 3 ? args[2].toInt() : 0);

    QCoreApplication app(argc, argv);
    return runCli(app.arguments().mid(1));
}
