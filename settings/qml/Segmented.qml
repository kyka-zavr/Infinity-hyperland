import QtQuick
import Infinity

// a few options in a row; model: [{value, label}]
Rectangle {
    id: root
    property var    model: []
    property var    value
    signal picked(var value)

    implicitHeight: 32
    implicitWidth: row.implicitWidth + 6
    radius: Style.radius
    color: Style.tint

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 2
        Repeater {
            model: root.model
            Rectangle {
                required property var modelData
                height: 26
                width: lbl.implicitWidth + 22
                radius: Style.radius - 2
                color: root.value === modelData.value ? Style.accent : (hov.containsMouse ? Style.tintHover : "transparent")
                Behavior on color { ColorAnimation { duration: 110 } }
                Text {
                    id: lbl
                    anchors.centerIn: parent
                    text: modelData.label
                    color: root.value === modelData.value ? Style.accentText : Style.text
                    font.pixelSize: 12
                }
                MouseArea {
                    id: hov
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.picked(modelData.value)
                }
            }
        }
    }
}
