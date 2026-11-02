import QtQuick
import Infinity

Column {
    id: page
    spacing: 14
    readonly property int rev: Backend.revision

    readonly property var labels: ({
        panel: "Panels (background)", bg: "Background", surface: "Surface", surface_hi: "Surface, selected", well: "Preview well",
        text: "Text", subtext: "Secondary text", accent: "Accent", accent_text: "Text on accent",
        border_active: "Window border, active", border_inactive: "Window border, inactive", shadow: "Shadow", danger: "Danger", success: "Success"
    })

    function themeModel() {
        const t = Backend.themes, m = []
        for (let i = 0; i < t.length; i++) m.push({value: t[i], label: t[i]})
        return m
    }

    Card {
        title: "Theme"
        SettingRow {
            title: "Color scheme"
            subtitle: "One palette for everything: the window switcher, this window, borders and shadows. Editing a color below makes your own copy named \"custom\"."
            Dropdown { width: 200; model: page.themeModel(); value: Backend.theme; onPicked: (v) => Backend.theme = v }
        }
        Column {
            width: parent.width
            Repeater {
                model: Backend.colorKeys()
                ColorField { required property string modelData; name: modelData; label: page.labels[modelData] || modelData }
            }
        }
    }

    Card {
        title: "Windows"
        SettingRow {
            title: "Manage these settings"
            subtitle: "Off: your own config decides how windows look. On: the values below (and the palette colors for borders and shadows) are applied."
            Toggle { checked: { page.rev; return Backend.get("appearance.enabled") } onToggled: (v) => Backend.set("appearance.enabled", v) }
        }
        Column {
            width: parent.width
            enabled: Backend.get("appearance.enabled")
            opacity: enabled ? 1 : 0.45
            SettingRow { title: "Inner gaps"; Slide { from: 0; to: 30; width: 180; value: { page.rev; return Backend.get("appearance.gaps_in") } onMoved: (v) => Backend.set("appearance.gaps_in", v) }
                         Text { text: Math.round(Backend.get("appearance.gaps_in")); color: Style.subtext; width: 28 } }
            SettingRow { title: "Outer gaps"; Slide { from: 0; to: 60; width: 180; value: { page.rev; return Backend.get("appearance.gaps_out") } onMoved: (v) => Backend.set("appearance.gaps_out", v) }
                         Text { text: Math.round(Backend.get("appearance.gaps_out")); color: Style.subtext; width: 28 } }
            SettingRow { title: "Border size"; Slide { from: 0; to: 8; width: 180; value: { page.rev; return Backend.get("appearance.border_size") } onMoved: (v) => Backend.set("appearance.border_size", v) }
                         Text { text: Math.round(Backend.get("appearance.border_size")); color: Style.subtext; width: 28 } }
            SettingRow { title: "Rounding"; Slide { from: 0; to: 30; width: 180; value: { page.rev; return Backend.get("appearance.rounding") } onMoved: (v) => Backend.set("appearance.rounding", v) }
                         Text { text: Math.round(Backend.get("appearance.rounding")); color: Style.subtext; width: 28 } }
            SettingRow { title: "Blur"; Toggle { checked: { page.rev; return Backend.get("appearance.blur") } onToggled: (v) => Backend.set("appearance.blur", v) } }
            SettingRow { title: "Shadows"; Toggle { checked: { page.rev; return Backend.get("appearance.shadow") } onToggled: (v) => Backend.set("appearance.shadow", v) } }
            SettingRow { title: "Borders use the palette"; subtitle: "Border colors come from the color scheme."
                         Toggle { checked: { page.rev; return Backend.get("appearance.theme_borders") } onToggled: (v) => Backend.set("appearance.theme_borders", v) } }
        }
    }

    Card {
        title: "Export to other programs"
        SettingRow {
            title: "Command after applying"
            subtitle: "Runs after every apply, for example to reload your bar or terminal. Palette templates live in ~/.config/infinity-hyprland/templates (see the documentation)."
            Field { width: 280; text: { page.rev; return Backend.get("export.post_command") } placeholder: "e.g. pkill -USR1 kitty"
                    onEdited: (t) => Backend.set("export.post_command", t) }
        }
    }
}
