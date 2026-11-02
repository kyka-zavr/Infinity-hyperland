import QtQuick
import Infinity

Rectangle {
    id: root
    property string text: ""
    property bool   primary: false
    property bool   danger: false
    property bool   flat: false
    signal clicked

    implicitWidth: label.implicitWidth + 28
    implicitHeight: 34
    radius: 8
    color: !enabled ? Style.surface
        : primary ? (area.containsMouse ? Qt.lighter(Style.accent, 1.1) : Style.accent)
        : danger ? (area.containsMouse ? Qt.lighter(Style.danger, 1.1) : Style.danger)
        : flat ? (area.containsMouse ? Style.surfaceHi : "transparent")
        : (area.containsMouse ? Style.surfaceHi : Style.surface)
    opacity: enabled ? 1 : 0.5

    Text {
        id: label
        anchors.centerIn: parent
        text: root.text
        color: (root.primary || root.danger) ? Style.accentText : Style.text
        font.pixelSize: 14
    }
    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
