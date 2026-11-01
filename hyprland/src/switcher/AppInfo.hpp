// Copyright (c) 2026, kyka-zavr.

#pragma once

#include <string>

// Looks a window up in the installed .desktop files: the human name of the application and the file of its icon.
namespace Switcher {
    struct SAppInfo {
        std::string name;     // "Firefox", never empty
        std::string iconPath; // png / svg file, empty when none was found
    };

    // windowClass is the app id / WM_CLASS of the window
    const SAppInfo& lookupApp(const std::string& windowClass);
}
