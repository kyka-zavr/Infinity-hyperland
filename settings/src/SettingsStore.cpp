// Copyright (c) 2026, kyka-zavr.
#include "SettingsStore.hpp"

#include "Paths.hpp"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QSaveFile>

QJsonObject SettingsStore::defaults() {
    static const char* JSON = R"({
  "version": 1,
  "theme": "dark",
  "infinity": { "pan_mods": "SUPER ALT", "zoom_mods": "CTRL", "zoom_step": 1.1 },
  "keyboard": { "layouts": [], "mode": "system", "default_layout": -1, "caps_mode": "normal" },
  "appearance": { "enabled": false, "theme_borders": true, "gaps_in": 5, "gaps_out": 20, "border_size": 2, "rounding": 10, "blur": true, "shadow": true },
  "binds": { "keys": {}, "disabled": [], "custom": [], "overrides": [] },
  "export": { "post_command": "" }
})";
    return QJsonDocument::fromJson(JSON).object();
}

// objects are merged key by key, everything else (values, arrays) is taken from the file when present
static QJsonObject mergeDefaults(const QJsonObject& file, const QJsonObject& defs) {
    QJsonObject out = file;
    for (auto it = defs.begin(); it != defs.end(); ++it) {
        if (!out.contains(it.key()))
            out[it.key()] = it.value();
        else if (it.value().isObject() && out[it.key()].isObject())
            out[it.key()] = mergeDefaults(out[it.key()].toObject(), it.value().toObject());
    }
    return out;
}

bool SettingsStore::load() {
    QFile f(Paths::settingsFile());
    if (!f.open(QIODevice::ReadOnly)) {
        m_root = defaults();
        return false;
    }

    QJsonParseError err;
    const auto      DOC = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !DOC.isObject()) {
        m_root = defaults();
        return false;
    }

    m_root = mergeDefaults(DOC.object(), defaults());
    return true;
}

bool SettingsStore::save() const {
    QDir().mkpath(Paths::configDir());

    QSaveFile f(Paths::settingsFile());
    if (!f.open(QIODevice::WriteOnly))
        return false;

    f.write(QJsonDocument(m_root).toJson(QJsonDocument::Indented));
    return f.commit();
}

QJsonValue SettingsStore::get(const QString& path) const {
    QJsonValue v = m_root;
    for (const auto& part : path.split('.')) {
        if (!v.isObject())
            return QJsonValue::Undefined;
        v = v.toObject().value(part);
    }
    return v;
}

static QJsonObject setIn(QJsonObject obj, const QStringList& parts, int idx, const QJsonValue& value) {
    if (idx == parts.size() - 1) {
        obj[parts[idx]] = value;
        return obj;
    }

    obj[parts[idx]] = setIn(obj.value(parts[idx]).toObject(), parts, idx + 1, value);
    return obj;
}

void SettingsStore::set(const QString& path, const QJsonValue& value) {
    m_root = setIn(m_root, path.split('.'), 0, value);
}

const QJsonObject& SettingsStore::root() const {
    return m_root;
}

void SettingsStore::setRoot(const QJsonObject& root) {
    m_root = mergeDefaults(root, defaults());
}
