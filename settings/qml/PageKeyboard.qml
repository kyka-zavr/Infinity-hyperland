import QtQuick
import Infinity

Column {
    id: page
    spacing: 14
    readonly property int rev: Backend.revision
    property string newLayout: ""
    property string newVariant: ""

    function layoutModel() {
        const l = Backend.layoutChoices(), m = []
        for (let i = 0; i < l.length; i++) m.push({value: l[i].code, label: l[i].name + "  (" + l[i].code + ")"})
        return m
    }
    function variantModel() {
        const l = Backend.layoutChoices(), m = [{value: "", label: "Default"}]
        for (let i = 0; i < l.length; i++) if (l[i].code === newLayout) for (let j = 0; j < l[i].variants.length; j++) m.push({value: l[i].variants[j].code, label: l[i].variants[j].name})
        return m
    }
    function defaultModel() {
        page.rev
        const l = Backend.configuredLayouts(), m = [{value: -1, label: "Keep the current one"}]
        for (let i = 0; i < l.length; i++) m.push({value: i, label: l[i].name})
        return m
    }

    Card {
        title: "Layouts"
        Repeater {
            model: { page.rev; return Backend.configuredLayouts() }
            SettingRow {
                required property var modelData
                required property int index
                title: modelData.name
                subtitle: modelData.layout + (modelData.variant !== "" ? " (" + modelData.variant + ")" : "") + (index === 0 ? "  · first" : "")
                Btn { text: "↑"; flat: true; enabled: index > 0; onClicked: Backend.moveLayout(index, -1) }
                Btn { text: "↓"; flat: true; enabled: index < Backend.configuredLayouts().length - 1; onClicked: Backend.moveLayout(index, 1) }
                Btn { text: "Remove"; flat: true; onClicked: Backend.removeLayout(index) }
            }
        }
        Text { visible: Backend.configuredLayouts().length === 0; text: "No layouts added: the keyboard settings of your own config are used."
               color: Style.subtext; font.pixelSize: 13; width: parent.width; wrapMode: Text.WordWrap; topPadding: 8; bottomPadding: 8 }
        SettingRow {
            title: "Add a layout"
            Dropdown { width: 260; model: page.layoutModel(); value: page.newLayout; placeholder: "Language..."; onPicked: (v) => { page.newLayout = v; page.newVariant = "" } }
            Dropdown { width: 200; model: page.variantModel(); value: page.newVariant; visible: page.newLayout !== ""; onPicked: (v) => page.newVariant = v }
            Btn { text: "Add"; primary: true; enabled: page.newLayout !== ""; onClicked: { Backend.addLayout(page.newLayout, page.newVariant); page.newLayout = ""; page.newVariant = "" } }
        }
    }

    Card {
        title: "Switching"
        SettingRow {
            title: "Layout is"
            subtitle: "Shared: one layout for the whole session. Per window: every window remembers its own, like Windows."
            Segmented {
                model: [{value: "system", label: "Shared"}, {value: "per_window", label: "Per window"}]
                value: { page.rev; return Backend.get("keyboard.mode") }
                onPicked: (v) => Backend.set("keyboard.mode", v)
            }
        }
        SettingRow {
            title: "Layout of a new window"
            subtitle: "Only in per-window mode."
            enabled: Backend.get("keyboard.mode") === "per_window"
            opacity: enabled ? 1 : 0.5
            Dropdown { width: 240; model: page.defaultModel(); value: { page.rev; return Backend.get("keyboard.default_layout") }
                       onPicked: (v) => Backend.set("keyboard.default_layout", v) }
        }
    }

    Card {
        title: "Num Lock"
        SettingRow {
            title: "Behaviour"
            subtitle: "Normal: unchanged. Per window: on in new windows, can be toggled, remembered for each window. Always on: everywhere."
            Segmented {
                model: [{value: "normal", label: "Normal"}, {value: "per_window", label: "Per window"}, {value: "always", label: "Always on"}]
                value: { page.rev; return Backend.get("keyboard.num_mode") }
                onPicked: (v) => Backend.set("keyboard.num_mode", v)
            }
        }
    }
}
