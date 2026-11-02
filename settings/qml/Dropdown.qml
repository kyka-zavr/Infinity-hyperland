import QtQuick
import QtQuick.Controls
import Infinity

// a list of choices in a popup; model: [{value, label}]
Item {
    id: root
    property var    model: []
    property var    value
    property string placeholder: "Choose..."
    signal picked(var value)

    implicitWidth: 240
    implicitHeight: 34

    function labelOf(v) {
        for (let i = 0; i < model.length; i++)
            if (model[i].value === v) return model[i].label
        return ""
    }

    Rectangle {
        anchors.fill: parent
        radius: 8
        color: area.containsMouse ? Style.surfaceHi : Style.well
        Text {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left; anchors.leftMargin: 12
            anchors.right: arrow.left; anchors.rightMargin: 6
            elide: Text.ElideRight
            text: root.labelOf(root.value) || root.placeholder
            color: root.labelOf(root.value) ? Style.text : Style.subtext
            font.pixelSize: 14
        }
        Text { id: arrow; anchors.right: parent.right; anchors.rightMargin: 12; anchors.verticalCenter: parent.verticalCenter; text: "▾"; color: Style.subtext }
        MouseArea { id: area; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: pop.open() }
    }

    Popup {
        id: pop
        y: root.height + 4
        width: Math.max(root.width, 260)
        height: Math.min(list.contentHeight + 12, 320)
        padding: 6
        background: Rectangle { radius: 10; color: Style.surface; border.color: Style.surfaceHi }
        ListView {
            id: list
            anchors.fill: parent
            clip: true
            model: root.model
            delegate: Rectangle {
                required property var modelData
                width: list.width; height: 32; radius: 6
                color: m.containsMouse ? Style.surfaceHi : "transparent"
                Text { anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 10; anchors.right: parent.right; anchors.rightMargin: 10
                       elide: Text.ElideRight; text: modelData.label; color: Style.text; font.pixelSize: 14 }
                MouseArea { id: m; anchors.fill: parent; hoverEnabled: true; onClicked: { pop.close(); root.picked(modelData.value) } }
            }
            ScrollBar.vertical: ScrollBar {}
        }
    }
}
