import QtQuick
import Infinity

Item {
    id: root
    property real from: 0
    property real to: 100
    property real value: 0
    property real step: 1
    signal moved(real value)

    implicitWidth: 240
    implicitHeight: 28

    Rectangle { id: track; anchors.verticalCenter: parent.verticalCenter; width: parent.width; height: 6; radius: 3; color: Style.well
        Rectangle { width: knob.x + knob.width / 2; height: parent.height; radius: 3; color: Style.accent } }
    Rectangle {
        id: knob
        width: 20; height: 20; radius: 10
        anchors.verticalCenter: parent.verticalCenter
        x: (root.value - root.from) / (root.to - root.from) * (root.width - width)
        color: Style.accent
        border.color: Style.accentText; border.width: 2
    }
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        function setFrom(mx) {
            let t = Math.max(0, Math.min(1, (mx - knob.width / 2) / (root.width - knob.width)))
            let v = root.from + t * (root.to - root.from)
            v = Math.round(v / root.step) * root.step
            root.moved(Math.max(root.from, Math.min(root.to, v)))
        }
        onPressed: (e) => setFrom(e.x)
        onPositionChanged: (e) => { if (pressed) setFrom(e.x) }
    }
}
