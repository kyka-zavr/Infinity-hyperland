import QtQuick
import Infinity

// a few options in a row; model: [{value, label}]
Rectangle {
    id: root
    property var    model: []
    property var    value
    signal picked(var value)

    implicitHeight: 34
    implicitWidth: row.implicitWidth + 8
    radius: 9
    color: Style.well

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 2
        Repeater {
            model: root.model
            Rectangle {
                required property var modelData
                height: 28
                width: lbl.implicitWidth + 24
                radius: 7
                color: root.value === modelData.value ? Style.accent : (hov.containsMouse ? Style.surfaceHi : "transparent")
                Text {
                    id: lbl
                    anchors.centerIn: parent
                    text: modelData.label
                    color: root.value === modelData.value ? Style.accentText : Style.text
                    font.pixelSize: 13
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
