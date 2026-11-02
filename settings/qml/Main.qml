import QtQuick
import QtQuick.Controls
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
        color: Style.surface

        Text { id: brand; text: "Infinity"; color: Style.accent; font.pixelSize: 22; font.bold: true; x: 22; y: 22 }
        Text { anchors.top: brand.bottom; x: 22; text: "settings"; color: Style.subtext; font.pixelSize: 13 }

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
                    width: parent.width; height: 42; radius: 9
                    color: win.page === index ? Style.accent : (m.containsMouse ? Style.surfaceHi : "transparent")
                    Text { x: 14; anchors.verticalCenter: parent.verticalCenter; text: modelData.icon; font.pixelSize: 17; color: win.page === index ? Style.accentText : Style.subtext }
                    Text { x: 44; anchors.verticalCenter: parent.verticalCenter; text: modelData.name; font.pixelSize: 15; color: win.page === index ? Style.accentText : Style.text }
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

        Text { id: title; text: win.pages[win.page].name; color: Style.text; font.pixelSize: 26; font.bold: true }
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
        color: Style.surface
        Text { anchors.verticalCenter: parent.verticalCenter; x: 18; text: Backend.status !== "" ? Backend.status : "Changes apply as you make them"; color: Style.subtext; font.pixelSize: 12 }
        Text { anchors.verticalCenter: parent.verticalCenter; anchors.right: parent.right; anchors.rightMargin: 18; text: "Theme: " + Backend.theme; color: Style.subtext; font.pixelSize: 12 }
    }
}
