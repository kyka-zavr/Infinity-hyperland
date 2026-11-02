import QtQuick
import QtQuick.Controls
import Infinity

Rectangle {
    id: root
    property alias text: input.text
    property alias placeholder: hint.text
    signal edited(string text)

    implicitWidth: 240
    implicitHeight: 34
    radius: 8
    color: Style.well
    border.width: input.activeFocus ? 2 : 0
    border.color: Style.accent

    Text { id: hint; anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 12; color: Style.subtext; font.pixelSize: 14; visible: input.text.length === 0 && !input.activeFocus }
    TextInput {
        id: input
        anchors.fill: parent
        anchors.leftMargin: 12; anchors.rightMargin: 12
        verticalAlignment: TextInput.AlignVCenter
        color: Style.text
        selectionColor: Style.accent
        selectedTextColor: Style.accentText
        font.pixelSize: 14
        clip: true
        onEditingFinished: root.edited(text)
    }
}
