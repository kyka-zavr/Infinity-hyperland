import QtQuick
import Infinity

// shows a key combination; a click starts capturing the next one
Rectangle {
    id: root
    property string keys: ""
    property bool   capturing: false
    signal captured(string keys)

    implicitWidth: 200
    implicitHeight: 32
    radius: 8
    color: capturing ? Style.accent : (area.containsMouse ? Style.surfaceHi : Style.well)
    focus: capturing

    function begin() { capturing = true; forceActiveFocus(); Backend.captureMode(true) }
    function stop() { if (capturing) { capturing = false; Backend.captureMode(false) } }

    Text {
        anchors.centerIn: parent
        text: root.capturing ? "Press the keys... (Esc cancels)" : (root.keys === "" ? "not set" : root.keys)
        color: root.capturing ? Style.accentText : (root.keys === "" ? Style.subtext : Style.text)
        font.pixelSize: 13
        elide: Text.ElideRight
        width: parent.width - 12
        horizontalAlignment: Text.AlignHCenter
    }
    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.begin()
    }
    Keys.onPressed: (e) => {
        if (!capturing) return
        e.accepted = true
        if (e.key === Qt.Key_Escape && e.modifiers === Qt.NoModifier) { stop(); return }
        const k = Backend.describeKey(e.key, e.modifiers)
        if (k === "") return          // only a modifier so far
        stop()
        root.captured(k)
    }
    onActiveFocusChanged: if (!activeFocus) stop()
    Component.onDestruction: stop()
}
