import QtQuick
import Infinity

// a palette entry: a swatch and a hex field (#rrggbb or #rrggbbaa)
Item {
    id: root
    property string name: ""
    property string label: ""

    implicitHeight: 40
    width: parent ? parent.width : 300

    Rectangle {
        id: sw
        width: 28; height: 28; radius: 7
        anchors.verticalCenter: parent.verticalCenter
        color: Style.c[root.name]
        border.width: 1; border.color: Style.surfaceHi
    }
    Text { anchors.left: sw.right; anchors.leftMargin: 12; anchors.verticalCenter: parent.verticalCenter; text: root.label; color: Style.text; font.pixelSize: 14 }
    Field {
        id: hex
        anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
        width: 130
        text: Backend.colorOf(root.name)
        onEdited: (t) => { Backend.setColor(root.name, t); hex.text = Backend.colorOf(root.name) }
    }
    Connections { target: Backend; function onThemeChanged() { hex.text = Backend.colorOf(root.name) } }
}
