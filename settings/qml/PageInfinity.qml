import QtQuick
import Infinity

Column {
    id: page
    spacing: 14
    readonly property int rev: Backend.revision

    Card {
        title: "Canvas"
        SettingRow {
            title: "Infinity mode"
            subtitle: "Switch the infinite canvas on or off on every monitor."
            Btn { text: "Toggle now"; primary: true; enabled: Backend.compositorRunning; onClicked: Backend.layoutMessage("infinity toggle") }
        }
        SettingRow {
            title: "Zoom step"
            subtitle: "How much one notch of the mouse wheel zooms."
            Slide { from: 1.02; to: 1.5; step: 0.01; width: 200
                    value: { page.rev; return Backend.get("infinity.zoom_step") }
                    onMoved: (v) => Backend.set("infinity.zoom_step", v) }
            Text { text: ((Backend.get("infinity.zoom_step") - 1) * 100).toFixed(0) + "%"; color: Style.subtext; width: 40; anchors.verticalCenter: parent.verticalCenter }
        }
        SettingRow {
            title: "Zoom with the wheel"
            subtitle: "Hold these keys and scroll."
            Segmented {
                model: [{value: "CTRL", label: "Ctrl"}, {value: "SUPER", label: "Super"}, {value: "ALT", label: "Alt"}, {value: "", label: "Off"}]
                value: { page.rev; return Backend.get("infinity.zoom_mods") }
                onPicked: (v) => Backend.set("infinity.zoom_mods", v)
            }
        }
        SettingRow {
            title: "Pan with the mouse"
            subtitle: "Hold these keys and move the mouse to move the whole canvas."
            Segmented {
                model: [{value: "SUPER ALT", label: "Super+Alt"}, {value: "CTRL ALT", label: "Ctrl+Alt"}, {value: "SUPER CTRL", label: "Super+Ctrl"}, {value: "", label: "Off"}]
                value: { page.rev; return Backend.get("infinity.pan_mods") }
                onPicked: (v) => Backend.set("infinity.pan_mods", v)
            }
        }
    }
}
