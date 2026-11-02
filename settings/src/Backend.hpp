// Copyright (c) 2026, kyka-zavr.
#pragma once

#include "SettingsStore.hpp"
#include "Theme.hpp"

#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

// The bridge between the QML pages and everything else: settings.json, themes, keyboard layouts, binds and system controls.
class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap colors READ colors NOTIFY themeChanged)
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(QStringList themes READ themes NOTIFY themeChanged)
    Q_PROPERTY(bool compositorRunning READ compositorRunning CONSTANT)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(int revision READ revision NOTIFY settingsChanged)

  public:
    explicit Backend(QObject* parent = nullptr);

    QVariantMap colors() const;
    QString     theme() const;
    void        setTheme(const QString& name);
    QStringList themes() const;
    bool        compositorRunning() const;
    QString     status() const;
    int         revision() const { return m_rev; }

    // generic settings access, dotted paths ("keyboard.mode")
    Q_INVOKABLE QVariant get(const QString& path) const;
    Q_INVOKABLE void     set(const QString& path, const QVariant& value);
    Q_INVOKABLE void     applyNow();
    Q_INVOKABLE void     layoutMessage(const QString& msg);   // e.g. "infinity toggle", sent to the running compositor

    // theme editing: builtin themes are copied to "custom" first
    Q_INVOKABLE void    setColor(const QString& key, const QString& hex);
    Q_INVOKABLE QString colorOf(const QString& key) const;
    Q_INVOKABLE QStringList colorKeys() const;

    // keyboard
    Q_INVOKABLE QVariantList layoutChoices() const;           // [{code, name, variants: [{code, name}]}]
    Q_INVOKABLE QVariantList configuredLayouts() const;       // [{layout, variant, name}]
    Q_INVOKABLE void         addLayout(const QString& layout, const QString& variant);
    Q_INVOKABLE void         removeLayout(int index);
    Q_INVOKABLE void         moveLayout(int index, int delta);

    // binds
    Q_INVOKABLE QVariantList managedBinds() const;            // [{id, label, keys, defaultKeys, disabled}]
    Q_INVOKABLE void         setManagedKeys(const QString& id, const QString& keys);
    Q_INVOKABLE void         setManagedDisabled(const QString& id, bool disabled);
    Q_INVOKABLE void         resetManaged(const QString& id);
    Q_INVOKABLE QVariantList customBinds() const;             // [{keys, kind, value, label}]
    Q_INVOKABLE void         addCustomBind(const QString& keys, const QString& kind, const QString& value, const QString& label);
    Q_INVOKABLE void         removeCustomBind(int index);
    Q_INVOKABLE QVariantList systemBinds() const;             // binds of the running compositor that are not ours: [{keys, what, description, mouse, state, to}]
    Q_INVOKABLE void         disableSystemBind(const QString& keys);
    Q_INVOKABLE void         rebindSystemBind(const QString& keys, const QString& to);
    Q_INVOKABLE void         restoreSystemBind(const QString& keys);
    Q_INVOKABLE QString      describeKey(int qtKey, int modifiers) const; // "" for a bare modifier
    Q_INVOKABLE void         captureMode(bool on);                       // no global binds fire while a key is being captured

    // system controls (wpctl, brightnessctl, nmcli: only when installed)
    Q_INVOKABLE QVariantMap  systemStatus() const;
    Q_INVOKABLE void         setVolume(int percent);
    Q_INVOKABLE void         setBrightness(int percent);
    Q_INVOKABLE void         setWifi(bool on);

  signals:
    void themeChanged();
    void statusChanged();
    void settingsChanged();

  private:
    void     changed();
    void     scheduleApply();
    QString  runTool(const QString& program, const QStringList& args, int timeoutMs = 3000) const;
    QString  hyprctl() const;
    void     saveTheme(const Theme& t);

    SettingsStore m_store;
    Theme         m_theme;
    QTimer        m_applyTimer;
    QString       m_status;
    int           m_rev = 0;
};
