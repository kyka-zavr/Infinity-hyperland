import QtQuick
import Infinity

Column {
    id: page
    spacing: 14
    property var st: ({})
    function refresh() { st = Backend.systemStatus() }
    Component.onCompleted: refresh()
    Timer { interval: 4000; running: true; repeat: true; onTriggered: page.refresh() }

    Card {
        title: "Sound"
        SettingRow {
            title: "Volume"
            subtitle: page.st.hasVolume ? "Output device" : "wpctl (PipeWire) was not found"
            enabled: !!page.st.hasVolume
            opacity: enabled ? 1 : 0.5
            Slide { width: 220; from: 0; to: 100; value: page.st.volume || 0; onMoved: (v) => { Backend.setVolume(v); page.st.volume = v; page.stChanged() } }
            Text { text: (page.st.volume || 0) + "%"; color: Style.subtext; width: 40 }
        }
    }
    Card {
        title: "Screen"
        SettingRow {
            title: "Brightness"
            subtitle: page.st.hasBrightness ? "Built-in display" : "brightnessctl was not found"
            enabled: !!page.st.hasBrightness
            opacity: enabled ? 1 : 0.5
            Slide { width: 220; from: 1; to: 100; value: page.st.brightness || 50; onMoved: (v) => { Backend.setBrightness(v); page.st.brightness = v; page.stChanged() } }
            Text { text: (page.st.brightness || 0) + "%"; color: Style.subtext; width: 40 }
        }
    }
    Card {
        title: "Network"
        SettingRow {
            title: "Wi-Fi"
            subtitle: page.st.hasNetwork ? (page.st.ssid ? "Connected to " + page.st.ssid : "Not connected") : "nmcli (NetworkManager) was not found"
            enabled: !!page.st.hasNetwork
            opacity: enabled ? 1 : 0.5
            Toggle { checked: !!page.st.wifi; onToggled: (v) => { Backend.setWifi(v); page.refresh() } }
        }
    }
}
