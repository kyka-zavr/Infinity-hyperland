// Copyright (c) 2026, kyka-zavr.
#include "Backend.hpp"

#include "Applier.hpp"
#include "LuaGenerator.hpp"
#include "Paths.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeySequence>
#include <QProcess>
#include <QUrl>
#include <QDesktopServices>
#include <QThread>
#include <QSysInfo>
#include <QStorageInfo>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>

Backend::Backend(QObject* parent) : QObject(parent) {
    m_store.load();
    QDir().mkpath(Paths::configDir());
    Theme::installBuiltins();
    m_theme = Theme::load(m_store.get("theme").toString("dark"));

    // the palette follows the file of another program (serpantinum): a change there is a change here
    connect(&m_watcher, &Watcher::themeSourceChanged, this, [this] {
        if (!m_theme.isDynamic())
            return;
        m_theme = Theme::load(m_store.get("theme").toString("dark"));
        emit themeChanged();
        scheduleApply();
    });
    connect(&m_watcher, &Watcher::settingsFileChanged, this, [this] {
        m_store.load();
        m_theme = Theme::load(m_store.get("theme").toString("dark"));
        changed();
        emit themeChanged();
    });

    m_applyTimer.setSingleShot(true);
    m_applyTimer.setInterval(600);
    connect(&m_applyTimer, &QTimer::timeout, this, &Backend::applyNow);
}

// ---------------------------------------------------------------- helpers

QString Backend::hyprctl() const {
    if (const auto E = qEnvironmentVariable("INFINITY_HYPRCTL"); !E.isEmpty())
        return E;

    const auto NEXT = QCoreApplication::applicationDirPath() + "/hyprctl";
    if (QFile::exists(NEXT))
        return NEXT;

    return QStandardPaths::findExecutable("hyprctl");
}

QString Backend::runTool(const QString& program, const QStringList& args, int timeoutMs) const {
    if (program.isEmpty())
        return {};

    QProcess p;
    p.start(program, args);
    if (!p.waitForFinished(timeoutMs))
        return {};
    return QString::fromUtf8(p.readAllStandardOutput());
}

void Backend::changed() {
    ++m_rev;
    emit settingsChanged();
}

void Backend::scheduleApply() {
    m_applyTimer.start();
}

void Backend::applyNow() {
    m_store.save();

    const auto R = Applier::apply({.reload = compositorRunning()});
    m_status     = R.ok ? (compositorRunning() ? "Applied" : "Saved (no running session to reload)") : R.messages.join("; ");
    emit statusChanged();
}

bool Backend::compositorRunning() const {
    return !qEnvironmentVariable("HYPRLAND_INSTANCE_SIGNATURE").isEmpty();
}

QString Backend::status() const {
    return m_status;
}

// ---------------------------------------------------------------- settings

QVariant Backend::get(const QString& path) const {
    return m_store.get(path).toVariant();
}

void Backend::set(const QString& path, const QVariant& value) {
    m_store.set(path, QJsonValue::fromVariant(value));
    changed();
    scheduleApply();
}

// ---------------------------------------------------------------- theme

static QString toQmlColor(const QString& rrggbbaa) { // "rrggbbaa" -> "#aarrggbb"
    return "#" + rrggbbaa.mid(6, 2) + rrggbbaa.left(6);
}

QVariantMap Backend::colors() const {
    QVariantMap m;
    for (const auto& k : Theme::colorKeys())
        m[k] = toQmlColor(m_theme.color(k));
    return m;
}

QString Backend::theme() const {
    return m_theme.name;
}

void Backend::setTheme(const QString& name) {
    m_store.set("theme", name);
    m_theme = Theme::load(name);
    m_theme.name = name;
    m_watcher.refresh();
    emit themeChanged();
    scheduleApply();
}

QStringList Backend::themes() const {
    return Theme::available();
}

QStringList Backend::colorKeys() const {
    return Theme::colorKeys();
}

QString Backend::colorOf(const QString& key) const {
    const auto C = m_theme.color(key);
    return C.mid(6, 2) == "ff" ? "#" + C.left(6) : "#" + C;
}

void Backend::saveTheme(const Theme& t) {
    QDir().mkpath(Paths::themesDir());
    QSaveFile f(Paths::themesDir() + "/" + t.name + ".json");
    if (f.open(QIODevice::WriteOnly)) {
        f.write(t.toJson());
        f.commit();
    }
}

void Backend::setColor(const QString& key, const QString& hex) {
    const auto N = Theme::normalize(hex);
    if (N.isEmpty())
        return;

    // builtin themes stay as they are: the first edit makes a copy named "custom"; a palette that follows a file
    // becomes a fixed snapshot (the edit would be overwritten by the next change of that file otherwise)
    if (Theme::builtinNames().contains(m_theme.name) || m_theme.isDynamic()) {
        m_theme.name = "custom";
        m_theme.importPath.clear();
        m_theme.importMap.clear();
        m_store.set("theme", "custom");
        m_watcher.refresh();
    }

    m_theme.colors[key] = N;
    saveTheme(m_theme);
    emit themeChanged();
    scheduleApply();
}

// ---------------------------------------------------------------- keyboard layouts

QVariantList Backend::layoutChoices() const {
    static QVariantList cache;
    if (!cache.isEmpty())
        return cache;

    QFile f("/usr/share/X11/xkb/rules/evdev.lst");
    if (!f.open(QIODevice::ReadOnly))
        return cache;

    QMap<QString, QString>                     names;
    QMap<QString, QVariantList>                variants;
    QString                                    section;
    static const QRegularExpression            RE(R"(^\s+(\S+)\s+(.*)$)");

    while (!f.atEnd()) {
        const auto LINE = QString::fromUtf8(f.readLine()).trimmed();
        if (LINE.startsWith('!')) {
            section = LINE.mid(1).trimmed();
            continue;
        }

        const auto M = RE.match("  " + LINE);
        if (!M.hasMatch())
            continue;

        if (section == "layout")
            names[M.captured(1)] = M.captured(2);
        else if (section == "variant") {
            // "intl   us: English (US, intl., with dead keys)"
            const auto REST  = M.captured(2);
            const auto COLON = REST.indexOf(':');
            if (COLON > 0)
                variants[REST.left(COLON).trimmed()].append(QVariantMap{{"code", M.captured(1)}, {"name", REST.mid(COLON + 1).trimmed()}});
        }
    }

    for (auto it = names.begin(); it != names.end(); ++it)
        cache.append(QVariantMap{{"code", it.key()}, {"name", it.value()}, {"variants", variants.value(it.key())}});

    std::sort(cache.begin(), cache.end(), [](const QVariant& a, const QVariant& b) { return a.toMap()["name"].toString() < b.toMap()["name"].toString(); });
    return cache;
}

QVariantList Backend::configuredLayouts() const {
    QVariantList out;
    for (const auto& v : m_store.get("keyboard.layouts").toArray()) {
        const auto O    = v.toObject();
        const auto CODE = O.value("layout").toString();
        QString    name = CODE;
        for (const auto& c : layoutChoices()) {
            if (c.toMap()["code"].toString() == CODE)
                name = c.toMap()["name"].toString();
        }
        out.append(QVariantMap{{"layout", CODE}, {"variant", O.value("variant").toString()}, {"name", name}});
    }
    return out;
}

void Backend::addLayout(const QString& layout, const QString& variant) {
    auto arr = m_store.get("keyboard.layouts").toArray();
    arr.append(QJsonObject{{"layout", layout}, {"variant", variant}});
    set("keyboard.layouts", arr.toVariantList());
}

void Backend::removeLayout(int index) {
    auto arr = m_store.get("keyboard.layouts").toArray();
    if (index < 0 || index >= arr.size())
        return;
    arr.removeAt(index);
    set("keyboard.layouts", arr.toVariantList());
}

void Backend::moveLayout(int index, int delta) {
    auto arr = m_store.get("keyboard.layouts").toArray();
    const int T = index + delta;
    if (index < 0 || index >= arr.size() || T < 0 || T >= arr.size())
        return;

    const auto V = arr.at(index);
    arr.removeAt(index);
    arr.insert(T, V);
    set("keyboard.layouts", arr.toVariantList());
}

// ---------------------------------------------------------------- binds

QVariantList Backend::managedBinds() const {
    const auto DISABLED = m_store.get("binds.disabled").toArray();

    QVariantList out;
    for (const auto& b : LuaGenerator::managedBinds()) {
        out.append(QVariantMap{{"id", b.id},
                               {"label", b.label},
                               {"keys", LuaGenerator::effectiveKeys(m_store, b)},
                               {"defaultKeys", b.keys},
                               {"disabled", DISABLED.contains(b.id)}});
    }
    return out;
}

void Backend::setManagedKeys(const QString& id, const QString& keys) {
    auto o = m_store.get("binds.keys").toObject();
    o[id]  = keys;
    m_store.set("binds.keys", o);

    auto d = m_store.get("binds.disabled").toArray();
    for (int i = 0; i < d.size(); ++i) {
        if (d.at(i).toString() == id) {
            d.removeAt(i);
            break;
        }
    }
    m_store.set("binds.disabled", d);

    changed();
    scheduleApply();
}

void Backend::setManagedDisabled(const QString& id, bool disabled) {
    auto d = m_store.get("binds.disabled").toArray();
    for (int i = 0; i < d.size(); ++i) {
        if (d.at(i).toString() == id)
            d.removeAt(i);
    }
    if (disabled)
        d.append(id);
    m_store.set("binds.disabled", d);

    changed();
    scheduleApply();
}

void Backend::resetManaged(const QString& id) {
    auto o = m_store.get("binds.keys").toObject();
    o.remove(id);
    m_store.set("binds.keys", o);
    setManagedDisabled(id, false);
}

QVariantList Backend::customBinds() const {
    QVariantList out;
    for (const auto& v : m_store.get("binds.custom").toArray())
        out.append(v.toObject().toVariantMap());
    return out;
}

void Backend::addCustomBind(const QString& keys, const QString& kind, const QString& value, const QString& label) {
    auto arr = m_store.get("binds.custom").toArray();
    arr.append(QJsonObject{{"keys", keys}, {"kind", kind}, {"value", value}, {"label", label}});
    m_store.set("binds.custom", arr);
    changed();
    scheduleApply();
}

void Backend::removeCustomBind(int index) {
    auto arr = m_store.get("binds.custom").toArray();
    if (index < 0 || index >= arr.size())
        return;
    arr.removeAt(index);
    m_store.set("binds.custom", arr);
    changed();
    scheduleApply();
}

static QString modmaskToString(int mask) {
    QStringList parts;
    if (mask & 64)
        parts << "SUPER";
    if (mask & 4)
        parts << "CTRL";
    if (mask & 8)
        parts << "ALT";
    if (mask & 1)
        parts << "SHIFT";
    return parts.join(" + ");
}

QVariantList Backend::systemBinds() const {
    QVariantList out;

    const auto OVERRIDES = m_store.get("binds.overrides").toArray();
    auto       overrideFor = [&](const QString& keys) -> QJsonObject {
        for (const auto& v : OVERRIDES) {
            if (v.toObject().value("keys").toString().compare(keys, Qt::CaseInsensitive) == 0)
                return v.toObject();
        }
        return {};
    };

    // the keys our own binds use are not "system" binds
    QStringList ours;
    for (const auto& b : LuaGenerator::managedBinds())
        ours << LuaGenerator::effectiveKeys(m_store, b).toUpper();
    for (const auto& v : m_store.get("binds.custom").toArray())
        ours << v.toObject().value("keys").toString().toUpper();

    QStringList seen;
    const auto  JSON = runTool(hyprctl(), {"binds", "-j"});
    for (const auto& v : QJsonDocument::fromJson(JSON.toUtf8()).array()) {
        const auto O = v.toObject();

        const auto MODS = modmaskToString(O.value("modmask").toInt());
        auto       key  = O.value("key").toString();
        if (key.isEmpty() && O.value("keycode").toInt() > 0)
            key = QString("code:%1").arg(O.value("keycode").toInt());

        const auto KEYS = MODS.isEmpty() ? key : MODS + " + " + key;
        if (ours.contains(KEYS.toUpper()))
            continue;

        QString what = O.value("dispatcher").toString();
        if (const auto A = what.indexOf('('); A >= 0 && what.endsWith(')'))
            what = what.mid(A + 1, what.size() - A - 2);

        // a bind that was moved by an override shows up under its old keys
        bool movedAway = false;
        for (const auto& ov : OVERRIDES) {
            if (ov.toObject().value("action").toString() == "rebind" && ov.toObject().value("to").toString().compare(KEYS, Qt::CaseInsensitive) == 0)
                movedAway = true;
        }
        if (movedAway)
            continue;

        seen << KEYS.toUpper();
        const auto OV = overrideFor(KEYS);
        out.append(QVariantMap{{"keys", KEYS},
                               {"what", what},
                               {"description", O.value("description").toString()},
                               {"mouse", O.value("mouse").toBool()},
                               {"state", OV.value("action").toString("normal")},
                               {"to", OV.value("to").toString()}});
    }

    // overrides whose bind is gone from the live list (disabled or moved) stay visible so they can be undone
    for (const auto& v : OVERRIDES) {
        const auto O    = v.toObject();
        const auto KEYS = O.value("keys").toString();
        if (seen.contains(KEYS.toUpper()))
            continue;

        out.append(QVariantMap{{"keys", KEYS}, {"what", O.value("action").toString() == "disable" ? "disabled by you" : "moved by you"}, {"description", ""}, {"mouse", false},
                               {"state", O.value("action").toString()}, {"to", O.value("to").toString()}});
    }

    return out;
}

static QJsonArray withoutOverride(const QJsonArray& arr, const QString& keys) {
    QJsonArray out;
    for (const auto& v : arr) {
        if (v.toObject().value("keys").toString().compare(keys, Qt::CaseInsensitive) != 0)
            out.append(v);
    }
    return out;
}

void Backend::disableSystemBind(const QString& keys) {
    auto arr = withoutOverride(m_store.get("binds.overrides").toArray(), keys);
    arr.append(QJsonObject{{"keys", keys}, {"action", "disable"}});
    m_store.set("binds.overrides", arr);
    changed();
    scheduleApply();
}

void Backend::rebindSystemBind(const QString& keys, const QString& to) {
    auto arr = withoutOverride(m_store.get("binds.overrides").toArray(), keys);
    arr.append(QJsonObject{{"keys", keys}, {"action", "rebind"}, {"to", to}});
    m_store.set("binds.overrides", arr);
    changed();
    scheduleApply();
}

void Backend::restoreSystemBind(const QString& keys) {
    m_store.set("binds.overrides", withoutOverride(m_store.get("binds.overrides").toArray(), keys));
    changed();
    scheduleApply();
}

QString Backend::describeKey(int qtKey, int modifiers) const {
    switch (qtKey) {
        case Qt::Key_Shift:
        case Qt::Key_Control:
        case Qt::Key_Alt:
        case Qt::Key_Meta:
        case Qt::Key_AltGr:
        case Qt::Key_CapsLock:
        case Qt::Key_Super_L:
        case Qt::Key_Super_R: return {};
        default: break;
    }

    QString key;
    if (qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z)
        key = QString(QChar(qtKey));
    else if (qtKey >= Qt::Key_0 && qtKey <= Qt::Key_9)
        key = QString(QChar(qtKey));
    else if (qtKey >= Qt::Key_F1 && qtKey <= Qt::Key_F24)
        key = QString("F%1").arg(qtKey - Qt::Key_F1 + 1);
    else {
        static const QMap<int, QString> NAMES = {
            {Qt::Key_Space, "SPACE"},  {Qt::Key_Return, "RETURN"}, {Qt::Key_Enter, "RETURN"}, {Qt::Key_Tab, "Tab"}, {Qt::Key_Backtab, "Tab"},
            {Qt::Key_Left, "left"},    {Qt::Key_Right, "right"},   {Qt::Key_Up, "up"},        {Qt::Key_Down, "down"}, {Qt::Key_Equal, "equal"},
            {Qt::Key_Plus, "equal"},   {Qt::Key_Minus, "minus"},   {Qt::Key_Comma, "comma"},  {Qt::Key_Period, "period"}, {Qt::Key_Slash, "slash"},
            {Qt::Key_Backslash, "backslash"}, {Qt::Key_Semicolon, "semicolon"}, {Qt::Key_Apostrophe, "apostrophe"}, {Qt::Key_BracketLeft, "bracketleft"},
            {Qt::Key_BracketRight, "bracketright"}, {Qt::Key_Escape, "Escape"}, {Qt::Key_Backspace, "BackSpace"}, {Qt::Key_Delete, "Delete"},
            {Qt::Key_Home, "Home"},    {Qt::Key_End, "End"},       {Qt::Key_PageUp, "Prior"}, {Qt::Key_PageDown, "Next"}, {Qt::Key_Print, "Print"},
            {Qt::Key_QuoteLeft, "grave"}, {Qt::Key_Insert, "Insert"},
        };
        key = NAMES.value(qtKey, QKeySequence(qtKey).toString());
    }
    if (key.isEmpty())
        return {};

    const auto M = modmaskToString(((modifiers & Qt::MetaModifier) ? 64 : 0) | ((modifiers & Qt::ControlModifier) ? 4 : 0) | ((modifiers & Qt::AltModifier) ? 8 : 0) |
                                   ((modifiers & Qt::ShiftModifier) ? 1 : 0));
    return M.isEmpty() ? key : M + " + " + key;
}

void Backend::captureMode(bool on) {
    if (m_capturing != on) {
        m_capturing = on;
        emit capturingChanged();
    }

    if (!compositorRunning())
        return;

    // an empty submap: no global bind fires, so the window receives the keys it is asked to capture
    runTool(hyprctl(), {"dispatch", on ? R"(hl.dsp.submap("infinity_capture"))" : R"(hl.dsp.submap("reset"))"});
}

// ---------------------------------------------------------------- system controls

QVariantMap Backend::systemStatus() const {
    QVariantMap m;

    const auto WPCTL = QStandardPaths::findExecutable("wpctl");
    m["hasVolume"]   = !WPCTL.isEmpty();
    if (!WPCTL.isEmpty()) {
        const auto OUT = runTool(WPCTL, {"get-volume", "@DEFAULT_AUDIO_SINK@"}).trimmed(); // "Volume: 0.45 [MUTED]"
        const auto M   = QRegularExpression(R"(Volume:\s*([0-9.]+))").match(OUT);
        m["volume"]    = M.hasMatch() ? qRound(M.captured(1).toDouble() * 100) : 0;
    }

    const auto BCTL     = QStandardPaths::findExecutable("brightnessctl");
    m["hasBrightness"]  = !BCTL.isEmpty();
    if (!BCTL.isEmpty()) {
        const auto F = runTool(BCTL, {"-m"}).trimmed().split(',');
        m["brightness"] = F.size() >= 4 ? F[3].chopped(1).toInt() : 0;
    }

    const auto NMCLI = QStandardPaths::findExecutable("nmcli");
    m["hasNetwork"]  = !NMCLI.isEmpty();
    if (!NMCLI.isEmpty()) {
        m["wifi"] = runTool(NMCLI, {"-t", "-f", "WIFI", "general"}).trimmed() == "enabled";
        QString ssid;
        for (const auto& l : runTool(NMCLI, {"-t", "-f", "ACTIVE,SSID", "dev", "wifi"}).split('\n')) {
            if (l.startsWith("yes:"))
                ssid = l.mid(4);
        }
        m["ssid"] = ssid;
    }

    return m;
}

void Backend::setVolume(int percent) {
    runTool(QStandardPaths::findExecutable("wpctl"), {"set-volume", "@DEFAULT_AUDIO_SINK@", QString("%1%").arg(percent)});
}

void Backend::setBrightness(int percent) {
    runTool(QStandardPaths::findExecutable("brightnessctl"), {"set", QString("%1%").arg(percent)});
}

void Backend::setWifi(bool on) {
    runTool(QStandardPaths::findExecutable("nmcli"), {"radio", "wifi", on ? "on" : "off"});
}

void Backend::layoutMessage(const QString& msg) {
    if (compositorRunning())
        runTool(hyprctl(), {"dispatch", QString("hl.dsp.layout(%1)").arg(LuaGenerator::quote(msg))});
}

// ---- updates ---------------------------------------------------------------------------------------------------

QString Backend::updaterPath() const {
    const QString next = QCoreApplication::applicationDirPath() + "/infinity-update";
    if (QFileInfo(next).isExecutable())
        return next;
    return QStandardPaths::findExecutable("infinity-update");
}

QVariantMap Backend::updateInfo() const {
    QVariantMap m{{"current", ""}, {"latest", ""}, {"available", false}, {"error", ""}, {"checked", 0}, {"installed", !updaterPath().isEmpty()}};
    QFile f(QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + "/infinity-settings/update.state");
    if (!f.open(QIODevice::ReadOnly))
        return m;
    for (const QByteArray& line : f.readAll().split('\n')) {
        const int eq = line.indexOf('=');
        if (eq <= 0)
            continue;
        const QString k = QString::fromUtf8(line.left(eq)), v = QString::fromUtf8(line.mid(eq + 1)).trimmed();
        if (k == "available")
            m[k] = (v == "1");
        else if (k == "checked")
            m[k] = v.toLongLong();
        else if (k == "current" || k == "latest" || k == "error")
            m[k] = v;
    }
    return m;
}

void Backend::checkUpdates() {
    const QString tool = updaterPath();
    if (tool.isEmpty())
        return;
    auto* p = new QProcess(this);
    connect(p, &QProcess::finished, this, [this, p] {
        p->deleteLater();
        emit updateChanged();
    });
    p->start(tool, {"check"});
}

bool Backend::startUpdate() {
    const QString tool = updaterPath();
    if (tool.isEmpty())
        return false;
    // the update asks for the sudo password and shows the build, so it needs a terminal
    const QString shellCmd = "'" + tool + "' run; echo; printf '%s' 'Enter: close... '; read _";
    QStringList candidates;
    if (const QString t = qEnvironmentVariable("TERMINAL"); !t.isEmpty())
        candidates << t;
    candidates << "kitty" << "foot" << "alacritty" << "wezterm" << "konsole" << "gnome-terminal" << "xfce4-terminal" << "xterm";
    for (const QString& c : candidates) {
        const QString exe = QStandardPaths::findExecutable(c);
        if (exe.isEmpty())
            continue;
        QStringList args;
        if (c == "gnome-terminal")
            args << "--" << "sh" << "-c" << shellCmd;
        else if (c == "wezterm")
            args << "start" << "--" << "sh" << "-c" << shellCmd;
        else
            args << "-e" << "sh" << "-c" << shellCmd;
        if (QProcess::startDetached(exe, args))
            return true;
    }
    return false;
}

// ---- about -----------------------------------------------------------------------------------------------------

static QString readFirstMatch(const QString& path, const QString& key) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    for (const QByteArray& line : f.readAll().split('\n')) {
        const int c = line.indexOf(':');
        if (c > 0 && QString::fromUtf8(line.left(c)).trimmed() == key)
            return QString::fromUtf8(line.mid(c + 1)).trimmed();
    }
    return {};
}

static QString uptimeText() {
    QFile f("/proc/uptime");
    if (!f.open(QIODevice::ReadOnly))
        return {};
    const qint64 s = static_cast<qint64>(f.readAll().split(' ').value(0).toDouble());
    const qint64 d = s / 86400, h = (s % 86400) / 3600, m = (s % 3600) / 60;
    QStringList parts;
    if (d)
        parts << QString("%1 d").arg(d);
    if (d || h)
        parts << QString("%1 h").arg(h);
    parts << QString("%1 min").arg(m);
    return parts.join(' ');
}

QVariantMap Backend::aboutInfo() const {
    auto row = [](const QString& label, const QString& value) { return QVariantMap{{"label", label}, {"value", value.isEmpty() ? QStringLiteral("Unknown") : value}}; };

    // installed version from the marker the installer writes: "version=0.57.0-infinity v0.57.0-infinity.3.1"
    QString version, base;
    QFile marker(QCoreApplication::applicationDirPath() + "/../.infinity-hyprland");
    if (marker.open(QIODevice::ReadOnly)) {
        for (const QByteArray& line : marker.readAll().split('\n'))
            if (line.startsWith("version=")) {
                const QStringList w = QString::fromUtf8(line.mid(8)).trimmed().split(' ');
                base    = w.value(0);
                version = w.last();
            }
    }

    QString cpu = readFirstMatch("/proc/cpuinfo", "model name");
    cpu.replace(QRegularExpression("\\s+"), " ");
    if (!cpu.isEmpty())
        cpu += QString(" (%1)").arg(QThread::idealThreadCount());

    QString gpu;
    for (const QString& l : runTool("lspci", {"-mm"}, 3000).split('\n')) {
        if (!l.contains("VGA compatible") && !l.contains("3D controller") && !l.contains("Display controller"))
            continue;
        const QStringList q = l.split('"');   // slot "class" "vendor" "device" ...
        if (q.size() > 5) {
            gpu = (q[3] + " " + q[5]).simplified();
            break;
        }
    }

    const double ramKb = readFirstMatch("/proc/meminfo", "MemTotal").split(' ').value(0).toDouble();
    const QStorageInfo root = QStorageInfo::root();
    const auto gb = [](qint64 bytes) { return QString::number(bytes / 1e9, 'f', 0); };

    QString shell = qEnvironmentVariable("SHELL");
    shell = shell.mid(shell.lastIndexOf('/') + 1);

    QVariantList hardware{
        row("Device name", QSysInfo::machineHostName()),
        row("Processor", cpu),
        row("Graphics", gpu),
        row("Memory", ramKb > 0 ? QString::number(ramKb / 1048576.0, 'f', 1) + " GB" : QString()),
        row("Disk", root.isValid() ? gb(root.bytesTotal() - root.bytesAvailable()) + " / " + gb(root.bytesTotal()) + " GB" : QString()),
    };
    QVariantList system{
        row("Operating system", QSysInfo::prettyProductName()),
        row("Kernel", QSysInfo::kernelVersion()),
        row("Desktop", qEnvironmentVariable("XDG_CURRENT_DESKTOP")),
        row("Shell", shell),
        row("Uptime", uptimeText()),
    };
    return {{"version", version}, {"base", base}, {"hardware", hardware}, {"system", system}};
}

void Backend::openUrl(const QString& url) {
    // the window must not become a way to open arbitrary links: only the project's own pages
    if (url.startsWith("https://github.com/kyka-zavr/Infinity-hyperland"))
        QDesktopServices::openUrl(QUrl(url));
}
