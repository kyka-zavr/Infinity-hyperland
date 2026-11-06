import QtQuick
import QtQuick.Shapes
import Infinity

Column {
    id: page
    spacing: 14
    property var info: ({})
    property var up: ({})
    function refresh() { info = Backend.aboutInfo() }
    function refreshUpdate() { up = Backend.updateInfo() }
    Component.onCompleted: { refresh(); refreshUpdate(); Backend.checkUpdates() }
    Connections { target: Backend; function onUpdateChanged() { page.refreshUpdate() } }
    Timer { interval: 30000; running: true; repeat: true; onTriggered: page.refresh() }

    // the title block: logo, name, version
    Item {
        width: parent.width
        height: 150
        Column {
            anchors.centerIn: parent
            spacing: 10
            Shape {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 150; height: 45
                preferredRendererType: Shape.CurveRenderer
                ShapePath {
                    fillColor: Style.accent
                    strokeColor: "transparent"
                    fillRule: ShapePath.OddEvenFill
                    scale: Qt.size(150 / 180, 150 / 180)
                    PathSvg { path: "M90 27C62 -9 0 -9 0 27C0 63 62 63 90 27C118 63 180 63 180 27C180 -9 118 -9 90 27ZM90 27C61 -2 20 3 20 27C20 51 61 56 90 27C110 47 160 51 160 27C160 3 110 7 90 27Z" }
                }
            }
            Text { anchors.horizontalCenter: parent.horizontalCenter; text: "Infinity-hyprland"; color: Style.text; font.pixelSize: 22; font.bold: true }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                color: Style.subtext; font.pixelSize: 13
                text: (page.info.version || page.up.current || "") + (page.info.base ? "  ·  based on Hyprland " + page.info.base.split("-")[0] : "")
            }
        }
    }

    Card {
        title: "Updates"
        SettingRow {
            title: page.up.available ? "Update available: " + page.up.latest : "Infinity-hyprland " + (page.up.current || "")
            subtitle: !page.up.installed ? "infinity-update was not found: reinstall with install.sh"
                    : page.up.available ? "You have " + page.up.current
                    : page.up.error === "network" ? "Could not reach GitHub"
                    : page.up.latest ? "You have the latest version" : "Not checked yet"
            Btn { text: "Changelog"; onClicked: Backend.openUrl("https://github.com/kyka-zavr/Infinity-hyperland/blob/main/CHANGELOG.md") }
            Btn { text: "Check"; enabled: !!page.up.installed; onClicked: Backend.checkUpdates() }
            Btn {
                text: "Update"
                primary: true
                visible: !!page.up.available && !!page.up.installed
                onClicked: if (!Backend.startUpdate()) updateNote.visible = true
            }
        }
        Text {
            id: updateNote
            visible: false
            width: parent.width
            wrapMode: Text.WordWrap
            color: Style.subtext
            font.pixelSize: 11
            text: "No terminal emulator found. Run in a terminal: infinity-update run"
        }
    }

    Card {
        title: "Hardware"
        Repeater {
            model: page.info.hardware || []
            SettingRow {
                required property var modelData
                implicitHeight: 40
                title: modelData.label
                Text { text: modelData.value; color: Style.subtext; font.pixelSize: 12; elide: Text.ElideRight; width: Math.min(implicitWidth, 420); horizontalAlignment: Text.AlignRight }
            }
        }
    }
    Card {
        title: "System"
        Repeater {
            model: page.info.system || []
            SettingRow {
                required property var modelData
                implicitHeight: 40
                title: modelData.label
                Text { text: modelData.value; color: Style.subtext; font.pixelSize: 12; elide: Text.ElideRight; width: Math.min(implicitWidth, 420); horizontalAlignment: Text.AlignRight }
            }
        }
    }
    Card {
        title: "Project"
        SettingRow {
            title: "GitHub"
            subtitle: "kyka-zavr/Infinity-hyperland"
            Btn { text: "Open"; onClicked: Backend.openUrl("https://github.com/kyka-zavr/Infinity-hyperland") }
        }
    }
}
