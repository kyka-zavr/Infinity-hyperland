import QtQuick
import Infinity

Rectangle {
    id: root
    property string text: ""
    property bool   primary: false
    property bool   danger: false
    property bool   flat: false
    signal clicked

    implicitWidth: label.implicitWidth + 26
    implicitHeight: 32
    radius: Style.radius
    color: !enabled ? Style.tint
        : primary ? (area.containsMouse ? Qt.lighter(Style.accent, 1.12) : Style.accent)
        : danger ? (area.containsMouse ? Qt.lighter(Style.danger, 1.12) : Style.danger)
        : flat ? (area.containsMouse ? Style.tintHover : "transparent")
        : (area.containsMouse ? Style.tintHover : Style.tint)
    opacity: enabled ? 1 : 0.5
    Behavior on color { ColorAnimation { duration: 110 } }

    Text {
        id: label
        anchors.centerIn: parent
        text: root.text
        color: (root.primary || root.danger) ? Style.accentText : Style.text
        font.pixelSize: 12
    }
    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
