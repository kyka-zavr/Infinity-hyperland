// Copyright (c) 2026, kyka-zavr.
#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QTimer>

// Watches the file a dynamic theme follows (for example the serpantinum colors) and settings.json; reports changes, debounced.
class Watcher : public QObject {
    Q_OBJECT
  public:
    explicit Watcher(QObject* parent = nullptr);

    // re-reads which files to watch (the theme can change)
    void refresh();

  signals:
    void themeSourceChanged();
    void settingsFileChanged();

  private:
    void rewatch();

    QFileSystemWatcher m_fs;
    QTimer             m_timer;
    bool               m_theme = false, m_settings = false;
};
