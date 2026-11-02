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
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>

Backend::Backend(QObject* parent) : QObject(parent) {
    m_store.load();
    QDir().mkpath(Paths::configDir());
    Theme::installBuiltins();
    m_theme = Theme::load(m_store.get("theme").toString("dark"));

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

    // builtin themes stay as they are: the first edit makes a copy named "custom"
    if (Theme::builtinNames().contains(m_theme.name)) {
        m_theme.name = "custom";
        m_store.set("theme", "custom");
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
