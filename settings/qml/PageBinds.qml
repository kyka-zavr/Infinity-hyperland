import QtQuick
import Infinity

Column {
    id: page
    spacing: 14
    readonly property int rev: Backend.revision
    property string newKeys: ""
    property string newKind: "exec"
    property string newValue: ""
    property string newLabel: ""
    property var    systemList: []
    property string rebinding: ""      // the keys of the system bind being rebound

    function refreshSystem() { systemList = Backend.systemBinds() }
    Component.onCompleted: refreshSystem()
    onRevChanged: refreshTimer.restart()
    Timer { id: refreshTimer; interval: 1200; onTriggered: page.refreshSystem() }

    Card {
        title: "Binds of this fork"
        Repeater {
            model: { page.rev; return Backend.managedBinds() }
            SettingRow {
                required property var modelData
                title: modelData.label
                subtitle: modelData.disabled ? "disabled" : (modelData.keys === modelData.defaultKeys ? "default" : "changed (default: " + modelData.defaultKeys + ")")
                KeyCapture { keys: modelData.disabled ? "" : modelData.keys; width: 210; onCaptured: (k) => Backend.setManagedKeys(modelData.id, k) }
                Btn { text: modelData.disabled ? "Enable" : "Disable"; flat: true; onClicked: Backend.setManagedDisabled(modelData.id, !modelData.disabled) }
                Btn { text: "Reset"; flat: true; enabled: modelData.disabled || modelData.keys !== modelData.defaultKeys; onClicked: Backend.resetManaged(modelData.id) }
            }
        }
    }

    Card {
        title: "Your own binds"
        Repeater {
            model: { page.rev; return Backend.customBinds() }
            SettingRow {
                required property var modelData
                required property int index
                title: modelData.label !== "" ? modelData.label : modelData.value
                subtitle: (modelData.kind === "lua" ? "Lua: " : "run: ") + modelData.value
                Text { text: modelData.keys; color: Style.text; anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 13 }
                Btn { text: "Remove"; flat: true; onClicked: Backend.removeCustomBind(index) }
            }
        }
        Item { width: 1; height: 8 }
        Text { text: "Add a bind"; color: Style.subtext; font.pixelSize: 12 }
        Row {
            spacing: 8
            topPadding: 6
            KeyCapture { keys: page.newKeys; width: 190; onCaptured: (k) => page.newKeys = k }
            Segmented { model: [{value: "exec", label: "Run a program"}, {value: "lua", label: "Lua action"}]; value: page.newKind; onPicked: (v) => page.newKind = v }
            Field { id: valueField; width: 230; text: page.newValue; placeholder: page.newKind === "exec" ? "command, e.g. firefox" : "e.g. hl.dsp.window.close()"; onEdited: (t) => page.newValue = t }
            Field { width: 140; text: page.newLabel; placeholder: "name (optional)"; onEdited: (t) => page.newLabel = t }
            Btn { text: "Add"; primary: true; enabled: page.newKeys !== "" && page.newValue !== ""
                  onClicked: { Backend.addCustomBind(page.newKeys, page.newKind, page.newValue, page.newLabel); page.newKeys = ""; page.newValue = ""; page.newLabel = "" } }
        }
        Item { width: 1; height: 10 }
    }

    Card {
        title: "Binds from your config (system)"
        Text { visible: !Backend.compositorRunning; text: "Start a session to see the binds of the running compositor."; color: Style.subtext; font.pixelSize: 13; topPadding: 6; bottomPadding: 10 }
        Text { visible: Backend.compositorRunning; width: parent.width; wrapMode: Text.WordWrap; color: Style.subtext; font.pixelSize: 12; topPadding: 4; bottomPadding: 8
               text: "Every bind the compositor knows, including the ones from your own Lua files. You can disable any of them or move it to other keys; your files are not changed, the choice is kept in settings.json." }
        Repeater {
            model: page.systemList
            SettingRow {
                required property var modelData
                title: modelData.keys + (modelData.state === "rebind" ? "  →  " + modelData.to : "")
                subtitle: (modelData.description !== "" ? modelData.description + " · " : "") + modelData.what + (modelData.state === "disable" ? "  · disabled" : "")
                KeyCapture { visible: page.rebinding === modelData.keys; width: 190
                             onVisibleChanged: if (visible) begin()
                             onCaptured: (k) => { Backend.rebindSystemBind(modelData.keys, k); page.rebinding = "" } }
                Btn { visible: modelData.state === "normal" && !modelData.mouse; text: "Move..."; flat: true; onClicked: page.rebinding = modelData.keys }
                Btn { visible: modelData.state === "normal"; text: "Disable"; flat: true; onClicked: Backend.disableSystemBind(modelData.keys) }
                Btn { visible: modelData.state !== "normal"; text: "Restore"; flat: true; onClicked: Backend.restoreSystemBind(modelData.keys) }
            }
        }
        Item { width: 1; height: 8 }
    }
}
