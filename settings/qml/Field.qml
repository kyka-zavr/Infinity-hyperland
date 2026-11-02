import QtQuick
import QtQuick.Controls
import Infinity

Rectangle {
    id: root
    property alias text: input.text
    property alias placeholder: hint.text
    signal edited(string text)

    implicitWidth: 240
    implicitHeight: 32
    radius: Style.radius
    color: Style.tint
    border.width: 1
    border.color: input.activeFocus ? Style.accent : "transparent"

    Text { id: hint; anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 12; color: Style.subtext; font.pixelSize: 12; visible: input.text.length === 0 && !input.activeFocus }
    TextInput {
        id: input
        anchors.fill: parent
        anchors.leftMargin: 12; anchors.rightMargin: 12
        verticalAlignment: TextInput.AlignVCenter
        color: Style.text
        selectionColor: Style.accent
        selectedTextColor: Style.accentText
        font.pixelSize: 12
        clip: true
        onEditingFinished: root.edited(text)
        Keys.onEscapePressed: (e) => { root.edited(text); focus = false; e.accepted = true }   // Esc leaves the field first, a second Esc closes the window
    }
}
