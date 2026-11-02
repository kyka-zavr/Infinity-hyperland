// Copyright (c) 2026, kyka-zavr.
#include "Theme.hpp"

#include "Paths.hpp"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

static const char* DARK = R"({"name":"dark","colors":{
  "panel":"#121217f0","bg":"#121217","surface":"#1f1f26","surface_hi":"#292b38","well":"#0d0d12",
  "text":"#ffffff","subtext":"#b8bdcc","accent":"#5c8fff","accent_text":"#ffffff",
  "border_active":"#5c8fff","border_inactive":"#595959aa","shadow":"#00000066","danger":"#ff5c5c","success":"#5cd68a"}})";

static const char* LIGHT = R"({"name":"light","colors":{
  "panel":"#f5f5f8f0","bg":"#f5f5f8","surface":"#e4e5ec","surface_hi":"#d3d6e4","well":"#ffffff",
  "text":"#1b1b22","subtext":"#5a5f70","accent":"#2f6bff","accent_text":"#ffffff",
  "border_active":"#2f6bff","border_inactive":"#b0b4c4aa","shadow":"#00000033","danger":"#d93636","success":"#1f9d55"}})";

QStringList Theme::colorKeys() {
    return {"panel", "bg", "surface", "surface_hi", "well", "text", "subtext", "accent", "accent_text", "border_active", "border_inactive", "shadow", "danger", "success"};
}

// follows the serpantinum rice (Catppuccin-style names); the file is written by its theme generator
static const char* SERPANTINUM = R"({"name":"serpantinum",
  "import":{"path":"~/.local/state/serpantinum/qs_colors.json","env":"QS_COLORS_JSON","map":{
    "panel":"mantle:f0","bg":"base","surface":"surface0","surface_hi":"surface1","well":"crust","text":"text","subtext":"subtext0",
    "accent":"blue","accent_text":"crust","border_active":"blue","border_inactive":"surface1:aa","shadow":"crust:66","danger":"red","success":"green"}},
  "colors":{"panel":"#121217f0","bg":"#121217","surface":"#1f1f26","surface_hi":"#292b38","well":"#0d0d12","text":"#ffffff","subtext":"#b8bdcc",
    "accent":"#5c8fff","accent_text":"#ffffff","border_active":"#5c8fff","border_inactive":"#595959aa","shadow":"#00000066","danger":"#ff5c5c","success":"#5cd68a"}})";

QStringList Theme::builtinNames() {
    QStringList n = {"dark", "light"};
    if (QFile::exists(fromJson(SERPANTINUM).importFile()))
        n << "serpantinum";   // only offered when that rice is installed
    return n;
}

QString Theme::importFile() const {
    if (!importEnv.isEmpty()) {
        if (const auto E = qEnvironmentVariable(importEnv.toUtf8().constData()); !E.isEmpty())
            return Paths::expandHome(E);
    }
    return Paths::expandHome(importPath);
}

void Theme::resolveImport() {
    if (!isDynamic())
        return;

    QFile f(importFile());
    if (!f.open(QIODevice::ReadOnly))
        return;

    const auto SRC = QJsonDocument::fromJson(f.readAll()).object();
    for (auto it = importMap.begin(); it != importMap.end(); ++it) {
        const auto SPEC  = it.value().split(':');
        const auto VALUE = normalize(SRC.value(SPEC[0]).toString());
        if (VALUE.isEmpty())
            continue;

        // an alpha in the spec replaces the one of the source color
        colors[it.key()] = SPEC.size() > 1 && SPEC[1].size() == 2 ? VALUE.left(6) + SPEC[1].toLower() : VALUE;
    }
}

QString Theme::normalize(const QString& text) {
    auto t = text.trimmed().toLower();
    if (t.startsWith('#'))
        t = t.mid(1);

    static const QString HEXCH = "0123456789abcdef";
    for (const auto c : t) {
        if (!HEXCH.contains(c))
            return {};
    }

    if (t.size() == 3) {
        QString o;
        for (const auto c : t)
            o += QString(2, c);
        return o + "ff";
    }
    if (t.size() == 6)
        return t + "ff";
    if (t.size() == 8)
        return t;
    return {};
}

QString Theme::color(const QString& key) const {
    if (colors.contains(key))
        return colors.value(key);

    // a theme may leave colors out: they come from the dark theme
    const auto DARKTHEME = fromJson(DARK);
    return DARKTHEME.colors.value(key, "ff00ffff");
}

QString Theme::hex(const QString& key) const {
    return "#" + color(key).left(6);
}

QString Theme::luaColor(const QString& key) const {
    return "rgba(" + color(key) + ")";
}

Theme Theme::fromJson(const QByteArray& json) {
    Theme       t;
    const auto  OBJ = QJsonDocument::fromJson(json).object();
    t.name          = OBJ.value("name").toString("custom");

    if (const auto IMP = OBJ.value("import").toObject(); !IMP.isEmpty()) {
        t.importPath = IMP.value("path").toString();
        t.importEnv  = IMP.value("env").toString();
        const auto MAP = IMP.value("map").toObject();
        for (auto it = MAP.begin(); it != MAP.end(); ++it)
            t.importMap[it.key()] = it.value().toString();
    }

    const auto COLORS = OBJ.value("colors").toObject();
    for (auto it = COLORS.begin(); it != COLORS.end(); ++it) {
        if (const auto N = normalize(it.value().toString()); !N.isEmpty())
            t.colors[it.key()] = N;
    }

    return t;
}

QByteArray Theme::toJson() const {
    QJsonObject colorsObj;
    for (const auto& k : colorKeys())
        colorsObj[k] = "#" + color(k);

    QJsonObject root{{"name", name}, {"colors", colorsObj}};
    if (isDynamic()) {
        QJsonObject map;
        for (auto it = importMap.begin(); it != importMap.end(); ++it)
            map[it.key()] = it.value();
        root["import"] = QJsonObject{{"path", importPath}, {"env", importEnv}, {"map", map}};
    }
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

Theme Theme::builtin(const QString& name) {
    auto t = fromJson(name == "light" ? LIGHT : (name == "serpantinum" ? SERPANTINUM : DARK));
    t.resolveImport();
    return t;
}

Theme Theme::load(const QString& name) {
    QFile f(Paths::themesDir() + "/" + name + ".json");
    if (f.open(QIODevice::ReadOnly)) {
        auto t = fromJson(f.readAll());
        if (t.name.isEmpty() || t.name == "custom")
            t.name = name;
        t.resolveImport();
        return t;
    }

    return builtin(name);
}

void Theme::installBuiltins() {
    QDir().mkpath(Paths::themesDir());
    for (const auto& n : builtinNames()) {
        const auto PATH = Paths::themesDir() + "/" + n + ".json";
        if (QFile::exists(PATH))
            continue;

        QSaveFile f(PATH);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(builtin(n).toJson());
            f.commit();
        }
    }
}

QStringList Theme::available() {
    QStringList names = builtinNames();
    for (const auto& f : QDir(Paths::themesDir()).entryList({"*.json"}, QDir::Files)) {
        const auto N = f.left(f.size() - 5);
        if (!names.contains(N))
            names << N;
    }
    return names;
}
