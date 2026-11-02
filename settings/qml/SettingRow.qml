import QtQuick
import Infinity

// a titled row: description on the left, the control on the right
Item {
    id: root
    property string title: ""
    property string subtitle: ""
    default property alias content: slot.data

    implicitHeight: Math.max(56, textCol.implicitHeight + 20)
    width: parent ? parent.width : 400

    Column {
        id: textCol
        anchors.left: parent.left
        anchors.right: slot.left; anchors.rightMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2
        Text { text: root.title; color: Style.text; font.pixelSize: 15; width: parent.width; elide: Text.ElideRight }
        Text { visible: root.subtitle !== ""; text: root.subtitle; color: Style.subtext; font.pixelSize: 12; width: parent.width; wrapMode: Text.WordWrap }
    }
    Row { id: slot; anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; spacing: 8 }
}
