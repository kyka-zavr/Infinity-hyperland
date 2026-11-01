// Copyright (c) 2026, kyka-zavr.

#include "AppInfo.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <unordered_map>
#include <vector>

using namespace Switcher;
namespace fs = std::filesystem;

namespace {
    struct SDesktopEntry {
        std::string name, icon;
    };

    std::string lower(std::string s) {
        std::ranges::transform(s, s.begin(), [](unsigned char c) { return std::tolower(c); });
        return s;
    }

    std::string trim(const std::string& s) {
        const auto B = s.find_first_not_of(" \t\r");
        if (B == std::string::npos)
            return {};
        return s.substr(B, s.find_last_not_of(" \t\r") - B + 1);
    }

    std::vector<fs::path> dataDirs() {
        std::vector<fs::path> dirs;

        if (const char* h = std::getenv("XDG_DATA_HOME"); h && *h)
            dirs.emplace_back(h);
        else if (const char* home = std::getenv("HOME"); home && *home)
            dirs.emplace_back(fs::path{home} / ".local/share");

        std::string x = "/usr/local/share:/usr/share";
        if (const char* d = std::getenv("XDG_DATA_DIRS"); d && *d)
            x = d;

        size_t start = 0;
        while (start <= x.size()) {
            const auto END = x.find(':', start);
            const auto PART = x.substr(start, END == std::string::npos ? std::string::npos : END - start);
            if (!PART.empty())
                dirs.emplace_back(PART);
            if (END == std::string::npos)
                break;
            start = END + 1;
        }

        return dirs;
    }

    // "ru_RU.UTF-8" -> {"ru_RU", "ru"}
    std::vector<std::string> localeKeys() {
        std::string l;
        for (const char* var : {"LC_ALL", "LC_MESSAGES", "LANG"}) {
            if (const char* v = std::getenv(var); v && *v) {
                l = v;
                break;
            }
        }

        std::vector<std::string> keys;
        if (l.empty() || l == "C" || l == "POSIX")
            return keys;

        l = l.substr(0, l.find_first_of(".@"));
        keys.emplace_back(l);
        if (const auto U = l.find('_'); U != std::string::npos)
            keys.emplace_back(l.substr(0, U));
        return keys;
    }

    struct SDatabase {
        std::vector<SDesktopEntry>                 entries;
        std::unordered_map<std::string, size_t>    byKey;  // lowercase file id / StartupWMClass -> entry (exact: wins)
        std::unordered_map<std::string, size_t>    byExec; // lowercase executable -> entry (a guess: only when nothing exact matched)
    };

    void addKey(SDatabase& db, const std::string& key, size_t idx, bool exec = false) {
        if (key.empty())
            return;
        (exec ? db.byExec : db.byKey).try_emplace(lower(key), idx); // earlier directories (user ones) win
    }

    void parseDesktopFile(SDatabase& db, const fs::path& file, const std::vector<std::string>& locales) {
        std::ifstream in{file};
        if (!in)
            return;

        SDesktopEntry entry;
        std::string   wmClass, exec, localizedName, name;
        bool          inEntry = false, hidden = false, noDisplay = false;

        std::string   line;
        while (std::getline(in, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#')
                continue;
            if (line[0] == '[') {
                inEntry = line == "[Desktop Entry]";
                continue;
            }
            if (!inEntry)
                continue;

            const auto EQ = line.find('=');
            if (EQ == std::string::npos)
                continue;
            const auto KEY = trim(line.substr(0, EQ));
            const auto VAL = trim(line.substr(EQ + 1));

            if (KEY == "Name")
                name = VAL;
            else if (KEY == "Icon")
                entry.icon = VAL;
            else if (KEY == "StartupWMClass")
                wmClass = VAL;
            else if (KEY == "Exec")
                exec = VAL;
            else if (KEY == "Hidden" && VAL == "true")
                hidden = true;
            else if (KEY == "NoDisplay" && VAL == "true")
                noDisplay = true;
            else if (localizedName.empty() && KEY.starts_with("Name[")) {
                for (const auto& l : locales) {
                    if (KEY == "Name[" + l + "]") {
                        localizedName = VAL;
                        break;
                    }
                }
            }
        }

        if (hidden)
            return;

        entry.name = !localizedName.empty() ? localizedName : name;
        if (entry.name.empty())
            return;

        const size_t IDX = db.entries.size();
        db.entries.emplace_back(entry);

        const auto ID = file.stem().string();
        addKey(db, ID, IDX);
        if (const auto DOT = ID.rfind('.'); DOT != std::string::npos)
            addKey(db, ID.substr(DOT + 1), IDX); // org.gnome.Nautilus -> Nautilus
        addKey(db, wmClass, IDX);

        // first word of Exec, without a path (env wrappers and field codes are not handled on purpose)
        if (!exec.empty()) {
            auto cmd = exec.substr(0, exec.find(' '));
            cmd      = cmd.substr(cmd.rfind('/') == std::string::npos ? 0 : cmd.rfind('/') + 1);
            if (cmd != "env" && cmd != "sh" && cmd != "bash")
                if (!noDisplay) // helper entries (URL launchers and the like) share the executable of the real application
                    addKey(db, cmd, IDX, true);
        }
    }

    SDatabase& database() {
        static SDatabase db = [] {
            SDatabase d;
            const auto LOCALES = localeKeys();

            for (const auto& base : dataDirs()) {
                std::error_code ec;
                for (fs::recursive_directory_iterator it{base / "applications", fs::directory_options::skip_permission_denied, ec}, end; !ec && it != end; it.increment(ec)) {
                    if (it->path().extension() == ".desktop")
                        parseDesktopFile(d, it->path(), LOCALES);
                }
            }

            return d;
        }();

        return db;
    }

    std::vector<std::string> iconThemes() {
        std::vector<std::string> themes;

        if (const char* home = std::getenv("HOME"); home && *home) {
            for (const char* rel : {".config/gtk-3.0/settings.ini", ".config/gtk-4.0/settings.ini"}) {
                std::ifstream in{fs::path{home} / rel};
                std::string   line;
                while (std::getline(in, line)) {
                    if (line.starts_with("gtk-icon-theme-name")) {
                        if (const auto EQ = line.find('='); EQ != std::string::npos)
                            themes.emplace_back(trim(line.substr(EQ + 1)));
                    }
                }
            }
        }

        for (const char* t : {"hicolor", "Papirus", "breeze", "Adwaita"})
            themes.emplace_back(t);
        return themes;
    }

    std::string findIcon(const std::string& icon) {
        if (icon.empty())
            return {};

        std::error_code ec;
        if (icon[0] == '/')
            return fs::exists(icon, ec) ? icon : std::string{};

        static const auto                  THEMES = iconThemes();
        static const std::vector<fs::path> BASES  = [] {
            std::vector<fs::path> b;
            if (const char* home = std::getenv("HOME"); home && *home) {
                b.emplace_back(fs::path{home} / ".icons");
                b.emplace_back(fs::path{home} / ".local/share/icons");
            }
            for (const auto& d : dataDirs())
                b.emplace_back(d / "icons");
            return b;
        }();

        // largest raster sizes first, vector icons last
        static const char* SIZES[] = {"256x256", "256", "128x128", "128", "96x96", "96", "64x64", "64", "48x48", "48", "scalable", "32x32", "32"};

        for (const auto& theme : THEMES) {
            for (const auto& base : BASES) {
                for (const char* size : SIZES) {
                    for (const char* ext : {".png", ".svg"}) {
                        for (const auto& p : {base / theme / size / "apps" / (icon + ext), base / theme / "apps" / size / (icon + ext)}) {
                            if (fs::exists(p, ec))
                                return p.string();
                        }
                    }
                }
            }
        }

        for (const char* ext : {".png", ".svg"}) {
            const fs::path p = fs::path{"/usr/share/pixmaps"} / (icon + ext);
            if (fs::exists(p, ec))
                return p.string();
        }

        return {};
    }
}

const SAppInfo& Switcher::lookupApp(const std::string& windowClass) {
    static std::unordered_map<std::string, SAppInfo> cache;

    if (const auto IT = cache.find(windowClass); IT != cache.end())
        return IT->second;

    SAppInfo info;
    info.name = windowClass.empty() ? "?" : windowClass;

    auto&      db  = database();
    const auto KEY = lower(windowClass);

    std::optional<size_t> idx;
    if (const auto IT = db.byKey.find(KEY); IT != db.byKey.end())
        idx = IT->second;
    else if (const auto IT2 = db.byExec.find(KEY); IT2 != db.byExec.end())
        idx = IT2->second;

    if (idx) {
        const auto& e = db.entries[*idx];
        info.name     = e.name;
        info.iconPath = findIcon(e.icon);
    }

    if (info.iconPath.empty())
        info.iconPath = findIcon(lower(windowClass)); // many apps are named after their class

    return cache.emplace(windowClass, std::move(info)).first->second;
}
