// Copyright (c) 2026, kyka-zavr.
pragma Singleton
import QtQuick
import Infinity

// every color comes from the active palette (Backend.colors), so the window follows the theme live
QtObject {
    readonly property var   c: Backend.colors
    readonly property color bg: c.bg
    readonly property color surface: c.surface
    readonly property color surfaceHi: c.surface_hi
    readonly property color well: c.well
    readonly property color text: c.text
    readonly property color subtext: c.subtext
    readonly property color accent: c.accent
    readonly property color accentText: c.accent_text
    readonly property color danger: c.danger
    readonly property color success: c.success
    readonly property int   radius: 10
    readonly property int   pad: 16
    readonly property string font: "sans-serif"
}
