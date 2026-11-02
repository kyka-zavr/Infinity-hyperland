import QtQuick
import QtQuick.Controls
import QtQuick.Shapes
import Infinity

ApplicationWindow {
    id: win
    width: 1040
    height: 720
    minimumWidth: 760
    minimumHeight: 520
    visible: true
    title: "Infinity Settings"
    color: Style.bg

    property int page: 0

    // Esc closes the window (not while a key is being captured; an open list or a text field takes Esc first)
    Shortcut { sequence: "Escape"; enabled: !Backend.capturing; onActivated: Qt.quit() }
    readonly property var pages: [
        { name: "Infinity", icon: "∞", source: "PageInfinity.qml" },
        { name: "Keyboard", icon: "⌨", source: "PageKeyboard.qml" },
        { name: "Appearance", icon: "◐", source: "PageAppearance.qml" },
        { name: "Keybinds", icon: "⌘", source: "PageBinds.qml" },
        { name: "System", icon: "⚙", source: "PageSystem.qml" }
    ]

    Rectangle {
        id: side
        width: 210
        anchors.top: parent.top; anchors.bottom: parent.bottom; anchors.left: parent.left
        color: Style.panel
        Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Style.line }

        // the logo as a vector shape, filled with the palette text colour
        Shape {
            id: brand
            x: 22; y: 24
            width: 120; height: 36
            preferredRendererType: Shape.CurveRenderer
            ShapePath {
                fillColor: Style.text
                strokeColor: "transparent"
                fillRule: ShapePath.OddEvenFill
                scale: Qt.size(120 / 180, 120 / 180)
                PathSvg { path: "M90 27C62 -9 0 -9 0 27C0 63 62 63 90 27C118 63 180 63 180 27C180 -9 118 -9 90 27ZM90 27C61 -2 20 3 20 27C20 51 61 56 90 27C110 47 160 51 160 27C160 3 110 7 90 27Z" }
            }
        }
        Text { anchors.top: brand.bottom; anchors.topMargin: 8; x: 22; text: "settings"; color: Style.subtext; font.pixelSize: 11; font.letterSpacing: 1.5 }

        Column {
            anchors.top: parent.top; anchors.topMargin: 88
            anchors.left: parent.left; anchors.right: parent.right
            anchors.leftMargin: 10; anchors.rightMargin: 10
            spacing: 4
            Repeater {
                model: win.pages
                Rectangle {
                    required property var modelData
                    required property int index
                    width: parent.width; height: 40; radius: Style.radius
                    color: win.page === index ? Style.accent : (m.containsMouse ? Style.tintHover : "transparent")
                    Behavior on color { ColorAnimation { duration: 110 } }
                    Text { x: 14; anchors.verticalCenter: parent.verticalCenter; text: modelData.icon; font.family: Style.iconFont; font.pixelSize: 16; color: win.page === index ? Style.accentText : Style.subtext }
                    Text { x: 44; anchors.verticalCenter: parent.verticalCenter; text: modelData.name; font.pixelSize: 13; color: win.page === index ? Style.accentText : Style.text }
                    MouseArea { id: m; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.page = index }
                }
            }
        }
    }

    Flickable {
        id: flick
        anchors.left: side.right; anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: bar.top
        anchors.margins: 22
        contentWidth: width
        contentHeight: loader.item ? loader.item.implicitHeight + 20 : 0
        clip: true
        ScrollBar.vertical: ScrollBar {}
        boundsBehavior: Flickable.StopAtBounds

        Text { id: title; text: win.pages[win.page].name; color: Style.text; font.pixelSize: 22; font.bold: true }
        Loader {
            id: loader
            y: title.height + 16
            width: flick.width - 14
            source: win.pages[win.page].source
        }
    }

    Rectangle {
        id: bar
        anchors.left: side.right; anchors.right: parent.right; anchors.bottom: parent.bottom
        height: 36
        color: Style.panel
        Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Style.line }
        Text { anchors.verticalCenter: parent.verticalCenter; x: 18; text: Backend.status !== "" ? Backend.status : "Changes apply as you make them"; color: Style.subtext; font.pixelSize: 11 }
        Text { anchors.verticalCenter: parent.verticalCenter; anchors.right: parent.right; anchors.rightMargin: 18; text: "Theme: " + Backend.theme + (Backend.themeFollowsFile ? "  (follows its source)" : ""); color: Style.subtext; font.pixelSize: 11 }
    }
}
