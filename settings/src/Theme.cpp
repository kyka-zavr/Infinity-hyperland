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

QStringList Theme::builtinNames() {
    return {"dark", "light"};
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

    return QJsonDocument(QJsonObject{{"name", name}, {"colors", colorsObj}}).toJson(QJsonDocument::Indented);
}

Theme Theme::builtin(const QString& name) {
    return fromJson(name == "light" ? LIGHT : DARK);
}

Theme Theme::load(const QString& name) {
    QFile f(Paths::themesDir() + "/" + name + ".json");
    if (f.open(QIODevice::ReadOnly)) {
        auto t = fromJson(f.readAll());
        if (t.name.isEmpty() || t.name == "custom")
            t.name = name;
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
