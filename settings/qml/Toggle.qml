import QtQuick
import Infinity

Rectangle {
    id: root
    property bool checked: false
    signal toggled(bool value)

    implicitWidth: 46
    implicitHeight: 26
    radius: 13
    color: checked ? Style.accent : Style.surfaceHi
    Behavior on color { ColorAnimation { duration: 120 } }

    Rectangle {
        width: 20; height: 20; radius: 10
        y: 3
        x: root.checked ? parent.width - width - 3 : 3
        color: root.checked ? Style.accentText : Style.subtext
        Behavior on x { NumberAnimation { duration: 120 } }
    }
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.toggled(!root.checked)
    }
}
