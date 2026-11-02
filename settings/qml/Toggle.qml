import QtQuick
import Infinity

Rectangle {
    id: root
    property bool checked: false
    signal toggled(bool value)

    implicitWidth: 44
    implicitHeight: 24
    radius: 12
    color: checked ? Style.accent : (area.containsMouse ? Style.tintHover : Style.tint)
    Behavior on color { ColorAnimation { duration: 130 } }

    Rectangle {
        width: 16; height: 16; radius: 8
        y: 4
        x: root.checked ? parent.width - width - 4 : 4
        color: root.checked ? Style.accentText : Style.text
        opacity: root.checked ? 1 : 0.75
        Behavior on x { NumberAnimation { duration: 140; easing.type: Easing.OutQuint } }
    }
    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.toggled(!root.checked)
    }
}
