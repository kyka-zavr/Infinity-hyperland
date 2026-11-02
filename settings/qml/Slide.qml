import QtQuick
import Infinity

// a progress bar you can drag, without a knob
Item {
    id: root
    property real from: 0
    property real to: 100
    property real value: 0
    property real step: 1
    signal moved(real value)

    implicitWidth: 240
    implicitHeight: 22

    readonly property real pos: Math.max(0, Math.min(1, (value - from) / Math.max(0.0001, to - from)))

    Rectangle {
        id: track
        anchors.fill: parent
        radius: height / 2
        color: Style.tint
        clip: true
        Rectangle {
            width: Math.max(parent.height, parent.width * root.pos)
            height: parent.height
            radius: parent.radius
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0; color: Style.accent }
                GradientStop { position: 1; color: Qt.lighter(Style.accent, 1.12) }
            }
            Behavior on width { NumberAnimation { duration: 90 } }
        }
    }
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        function setFrom(mx) {
            let t = Math.max(0, Math.min(1, mx / root.width))
            let v = root.from + t * (root.to - root.from)
            v = Math.round(v / root.step) * root.step
            root.moved(Math.max(root.from, Math.min(root.to, v)))
        }
        onPressed: (e) => setFrom(e.x)
        onPositionChanged: (e) => { if (pressed) setFrom(e.x) }
    }
}
