// Copyright (c) 2026, kyka-zavr.
#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QString>

// settings.json: the one model of everything configurable (see docs/en/design-settings.md).
class SettingsStore {
  public:
    // reads the file; a missing or broken file gives the defaults. Missing keys are always filled from the defaults.
    bool load();
    bool save() const;

    // dotted path, e.g. "keyboard.mode"
    QJsonValue          get(const QString& path) const;
    void                set(const QString& path, const QJsonValue& value);

    const QJsonObject&  root() const;
    void                setRoot(const QJsonObject& root);
    static QJsonObject  defaults();

  private:
    QJsonObject m_root = defaults();
};
