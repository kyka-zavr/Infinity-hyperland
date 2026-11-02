// Copyright (c) 2026, kyka-zavr.
#include "Watcher.hpp"

#include "Paths.hpp"
#include "SettingsStore.hpp"
#include "Theme.hpp"

#include <QFileInfo>

Watcher::Watcher(QObject* parent) : QObject(parent) {
    m_timer.setSingleShot(true);
    m_timer.setInterval(700);

    connect(&m_fs, &QFileSystemWatcher::fileChanged, this, [this](const QString& path) {
        (path == Paths::settingsFile() ? m_settings : m_theme) = true;
        m_timer.start();
    });
    connect(&m_fs, &QFileSystemWatcher::directoryChanged, this, [this](const QString&) {
        m_theme = true; // a file that was replaced (atomic write) shows up as a directory change
        m_timer.start();
    });
    connect(&m_timer, &QTimer::timeout, this, [this] {
        rewatch(); // replaced files lose their watch
        if (m_settings)
            emit settingsFileChanged();
        if (m_theme)
            emit themeSourceChanged();
        m_theme = m_settings = false;
    });

    refresh();
}

void Watcher::refresh() {
    rewatch();
}

void Watcher::rewatch() {
    if (!m_fs.files().isEmpty())
        m_fs.removePaths(m_fs.files());
    if (!m_fs.directories().isEmpty())
        m_fs.removePaths(m_fs.directories());

    SettingsStore store;
    store.load();

    QStringList paths = {Paths::settingsFile()};
    const auto  THEME = Theme::load(store.get("theme").toString("dark"));
    if (THEME.isDynamic()) {
        paths << THEME.importFile() << QFileInfo(THEME.importFile()).absolutePath();
    }

    for (const auto& p : paths) {
        if (QFileInfo::exists(p))
            m_fs.addPath(p);
    }
}
