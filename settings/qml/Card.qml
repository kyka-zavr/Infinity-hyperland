import QtQuick
import Infinity

Rectangle {
    id: root
    default property alias content: col.data
    property string title: ""

    width: parent ? parent.width : 400
    implicitHeight: col.implicitHeight + (title !== "" ? 56 : 24)
    radius: Style.radius + 2
    color: Style.surface
    border.width: 1
    border.color: Style.line

    Text { visible: root.title !== ""; text: root.title; color: Style.subtext; font.pixelSize: 11; font.bold: true; font.letterSpacing: 1.2; font.capitalization: Font.AllUppercase
           anchors.left: parent.left; anchors.leftMargin: 18; anchors.top: parent.top; anchors.topMargin: 16 }
    Column {
        id: col
        anchors.left: parent.left; anchors.right: parent.right
        anchors.leftMargin: 18; anchors.rightMargin: 18
        anchors.top: parent.top; anchors.topMargin: root.title !== "" ? 40 : 12
        spacing: 0
    }
}
