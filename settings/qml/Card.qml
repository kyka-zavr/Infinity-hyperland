import QtQuick
import Infinity

Rectangle {
    id: root
    default property alias content: col.data
    property string title: ""

    width: parent ? parent.width : 400
    implicitHeight: col.implicitHeight + (title !== "" ? 56 : 24)
    radius: Style.radius
    color: Style.surface

    Text { visible: root.title !== ""; text: root.title; color: Style.accent; font.pixelSize: 13; font.bold: true; font.capitalization: Font.AllUppercase
           anchors.left: parent.left; anchors.leftMargin: 18; anchors.top: parent.top; anchors.topMargin: 16 }
    Column {
        id: col
        anchors.left: parent.left; anchors.right: parent.right
        anchors.leftMargin: 18; anchors.rightMargin: 18
        anchors.top: parent.top; anchors.topMargin: root.title !== "" ? 40 : 12
        spacing: 0
    }
}
