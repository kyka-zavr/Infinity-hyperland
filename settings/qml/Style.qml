// Copyright (c) 2026, kyka-zavr.
pragma Singleton
import QtQuick
import Infinity

// every color comes from the active palette (Backend.colors), so the window follows the theme live.
// The look (monospace font, radius 8, translucent controls, bar sliders) follows the serpantinum rice.
QtObject {
    readonly property var   c: Backend.colors
    readonly property color bg: c.bg
    readonly property color panel: c.panel
    readonly property color surface: c.surface
    readonly property color surfaceHi: c.surface_hi
    readonly property color well: c.well
    readonly property color text: c.text
    readonly property color subtext: c.subtext
    readonly property color accent: c.accent
    readonly property color accentText: c.accent_text
    readonly property color danger: c.danger
    readonly property color success: c.success
    // a faint tint of the text color: the translucent base of controls
    readonly property color tint: Qt.rgba(text.r, text.g, text.b, 0.09)
    readonly property color tintHover: Qt.rgba(text.r, text.g, text.b, 0.16)
    readonly property color line: Qt.rgba(text.r, text.g, text.b, 0.12)
    readonly property int   radius: 8
    readonly property int   pad: 16
    readonly property string font: "Adwaita Mono"
    readonly property string iconFont: "Iosevka Nerd Font"
}
