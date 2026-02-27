import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: settingsPanel
    color: theme.background
    signal closed()

    Flickable {
        anchors.fill: parent
        contentHeight: settingsCol.implicitHeight + 48
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            contentItem: Rectangle { implicitWidth: 4; radius: 2; color: theme.borderFocus }
            background: Item {}
        }

        ColumnLayout {
            id: settingsCol
            anchors {
                left: parent.left; right: parent.right
                top: parent.top
                leftMargin: 48; rightMargin: 48; topMargin: 32
            }
            spacing: 0

            // Header
            RowLayout {
                Layout.fillWidth: true
                Layout.bottomMargin: 32
                Text {
                    text: "⚙ Settings"
                    font.family: settings.fontFamily
                    font.pixelSize: 24; font.weight: Font.Bold
                    color: theme.text
                    Layout.fillWidth: true
                }
                Rectangle {
                    width: 28; height: 28; radius: 6
                    color: closeMa.containsMouse ? theme.hover : "transparent"
                    Text {
                        anchors.centerIn: parent; text: "✕"
                        font.pixelSize: 16; color: theme.textDim
                    }
                    MouseArea {
                        id: closeMa; anchors.fill: parent
                        hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: settingsPanel.closed()
                    }
                }
            }

            // ---- EDITOR SECTION ----
            SectionHeader { text: "EDITOR" }

            SettingRow {
                label: "Font Family"
                SettingTextField {
                    text: settings.fontFamily
                    onEditingFinished: settings.fontFamily = text
                }
            }

            SettingRow {
                label: "Font Size"
                RowLayout {
                    spacing: 8
                    SettingButton { text: "−"; onClicked: settings.fontSize = Math.max(8, settings.fontSize - 1) }
                    Text {
                        text: settings.fontSize + "px"
                        font.family: settings.fontFamily; font.pixelSize: 13
                        color: theme.text; Layout.preferredWidth: 40
                        horizontalAlignment: Text.AlignHCenter
                    }
                    SettingButton { text: "+"; onClicked: settings.fontSize = Math.min(32, settings.fontSize + 1) }
                }
            }

            SettingRow {
                label: "Tab Width"
                RowLayout {
                    spacing: 8
                    SettingButton { text: "−"; onClicked: settings.tabWidth = Math.max(1, settings.tabWidth - 1) }
                    Text {
                        text: settings.tabWidth + " spaces"
                        font.family: settings.fontFamily; font.pixelSize: 13
                        color: theme.text; Layout.preferredWidth: 64
                        horizontalAlignment: Text.AlignHCenter
                    }
                    SettingButton { text: "+"; onClicked: settings.tabWidth = Math.min(8, settings.tabWidth + 1) }
                }
            }

            SettingRow {
                label: "Word Wrap"
                SettingToggle { checked: settings.wordWrap; onToggled: settings.wordWrap = checked }
            }

            SettingRow {
                label: "Line Numbers"
                SettingToggle { checked: settings.showLineNumbers; onToggled: settings.showLineNumbers = checked }
            }

            // ---- LSP SECTION ----
            SectionHeader { text: "LANGUAGE SERVER PROTOCOL"; Layout.topMargin: 24 }

            SettingRow {
                label: "Enable LSP"
                SettingToggle { checked: settings.lspEnabled; onToggled: settings.lspEnabled = checked }
            }

            SettingRow {
                label: "Auto-start on file open"
                SettingToggle {
                    checked: settings.lspAutostart
                    onToggled: settings.lspAutostart = checked
                    enabled: settings.lspEnabled
                }
            }

            Text {
                text: "Server Paths (leave blank for defaults)"
                font.family: settings.fontFamily; font.pixelSize: 11
                color: theme.textMuted
                Layout.topMargin: 12; Layout.bottomMargin: 8
            }

            SettingRow {
                label: "C/C++ (clangd)"
                SettingTextField {
                    text: settings.lspCpp
                    placeholderText: "clangd"
                    onEditingFinished: settings.lspCpp = text
                    enabled: settings.lspEnabled
                }
            }

            SettingRow {
                label: "Python (pylsp)"
                SettingTextField {
                    text: settings.lspPython
                    placeholderText: "pylsp"
                    onEditingFinished: settings.lspPython = text
                    enabled: settings.lspEnabled
                }
            }

            SettingRow {
                label: "Rust (rust-analyzer)"
                SettingTextField {
                    text: settings.lspRust
                    placeholderText: "rust-analyzer"
                    onEditingFinished: settings.lspRust = text
                    enabled: settings.lspEnabled
                }
            }

            SettingRow {
                label: "JS/TS"
                SettingTextField {
                    text: settings.lspJs
                    placeholderText: "typescript-language-server"
                    onEditingFinished: settings.lspJs = text
                    enabled: settings.lspEnabled
                }
            }

            SettingRow {
                label: "Go (gopls)"
                SettingTextField {
                    text: settings.lspGo
                    placeholderText: "gopls"
                    onEditingFinished: settings.lspGo = text
                    enabled: settings.lspEnabled
                }
            }

            // ---- MATUGEN / THEME SECTION ----
            SectionHeader { text: "THEME — MATUGEN"; Layout.topMargin: 24 }

            SettingRow {
                label: "Enable Matugen Colors"
                SettingToggle { checked: theme.matugenEnabled; onToggled: theme.matugenEnabled = checked }
            }

            SettingRow {
                label: "Colors JSON Path"
                SettingTextField {
                    text: theme.matugenColorsPath
                    placeholderText: "~/.cache/matugen/colors.json"
                    onEditingFinished: theme.matugenColorsPath = text
                    enabled: theme.matugenEnabled
                    Layout.fillWidth: true
                }
            }

            SettingRow {
                label: "Color Scheme"
                RowLayout {
                    spacing: 6
                    Repeater {
                        model: ["dark", "light", "amoled"]
                        delegate: Rectangle {
                            width: schemeLabel.implicitWidth + 20; height: 28; radius: 4
                            color: theme.matugenScheme === modelData ? theme.accent : theme.surface
                            border.color: theme.matugenScheme === modelData ? theme.accent : theme.border
                            border.width: 1
                            Text {
                                id: schemeLabel; anchors.centerIn: parent
                                text: modelData
                                font.family: settings.fontFamily; font.pixelSize: 11
                                color: theme.matugenScheme === modelData ? theme.background : theme.textDim
                            }
                            MouseArea {
                                anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                onClicked: theme.matugenScheme = modelData
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.topMargin: 12
                spacing: 8
                Rectangle {
                    width: reloadLabel.implicitWidth + 20; height: 32; radius: 6
                    color: reloadMa.containsMouse ? theme.surfaceAlt : theme.surface
                    border.color: theme.border; border.width: 1
                    Text {
                        id: reloadLabel; anchors.centerIn: parent
                        text: "↻ Reload Matugen Colors"
                        font.family: settings.fontFamily; font.pixelSize: 12
                        color: theme.accent
                    }
                    MouseArea {
                        id: reloadMa; anchors.fill: parent
                        hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: theme.reloadMatugen()
                    }
                }
                Rectangle {
                    width: resetLabel.implicitWidth + 20; height: 32; radius: 6
                    color: resetMa.containsMouse ? theme.surfaceAlt : theme.surface
                    border.color: theme.border; border.width: 1
                    Text {
                        id: resetLabel; anchors.centerIn: parent
                        text: "Reset to Defaults"
                        font.family: settings.fontFamily; font.pixelSize: 12
                        color: theme.textDim
                    }
                    MouseArea {
                        id: resetMa; anchors.fill: parent
                        hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: theme.resetToDefaults()
                    }
                }
            }

            // Color preview
            Text {
                text: "CURRENT THEME PREVIEW"
                font.family: settings.fontFamily; font.pixelSize: 11
                font.letterSpacing: 2; color: theme.textMuted
                Layout.topMargin: 24; Layout.bottomMargin: 8
            }

            Flow {
                Layout.fillWidth: true
                spacing: 8
                Repeater {
                    model: [
                        { name: "bg", color: theme.background },
                        { name: "bgAlt", color: theme.backgroundAlt },
                        { name: "surface", color: theme.surface },
                        { name: "surfAlt", color: theme.surfaceAlt },
                        { name: "border", color: theme.border },
                        { name: "accent", color: theme.accent },
                        { name: "text", color: theme.text },
                        { name: "textDim", color: theme.textDim },
                        { name: "muted", color: theme.textMuted },
                        { name: "error", color: theme.error },
                        { name: "warning", color: theme.warning },
                        { name: "success", color: theme.success },
                    ]
                    delegate: ColumnLayout {
                        spacing: 2
                        Rectangle {
                            width: 48; height: 32; radius: 4
                            color: modelData.color
                            border.color: theme.border; border.width: 1
                        }
                        Text {
                            text: modelData.name
                            font.family: settings.fontFamily; font.pixelSize: 9
                            color: theme.textMuted
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: 48 }
        }
    }

    // --- Reusable sub-components ---

    component SectionHeader: Text {
        font.family: settings.fontFamily; font.pixelSize: 11
        font.letterSpacing: 3; color: theme.textMuted
        Layout.fillWidth: true; Layout.bottomMargin: 12; Layout.topMargin: 8
        Rectangle {
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom; bottomMargin: -6 }
            height: 1; color: theme.border
        }
    }

    component SettingRow: RowLayout {
        default property alias content: contentSlot.data
        property string label: ""
        Layout.fillWidth: true
        Layout.bottomMargin: 8
        spacing: 16
        Text {
            text: parent.label
            font.family: settings.fontFamily; font.pixelSize: 13
            color: theme.textDim
            Layout.preferredWidth: 180
        }
        Item {
            id: contentSlot
            Layout.fillWidth: true
            implicitHeight: childrenRect.height
            implicitWidth: childrenRect.width
        }
    }

    component SettingTextField: TextField {
        font.family: settings.fontFamily; font.pixelSize: 12
        color: theme.text; placeholderTextColor: theme.textMuted
        Layout.fillWidth: true
        padding: 6
        background: Rectangle {
            radius: 4; color: theme.surface
            border.color: parent.activeFocus ? theme.accent : theme.border
            border.width: 1
        }
    }

    component SettingToggle: Rectangle {
        property bool checked: false
        signal toggled()
        width: 40; height: 22; radius: 11
        color: checked ? theme.accent : theme.border
        Behavior on color { ColorAnimation { duration: 150 } }
        Rectangle {
            width: 16; height: 16; radius: 8
            x: parent.checked ? 21 : 3; y: 3
            color: parent.checked ? theme.background : theme.textDim
            Behavior on x { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
        }
        MouseArea {
            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
            onClicked: { parent.checked = !parent.checked; parent.toggled() }
        }
    }

    component SettingButton: Rectangle {
        property alias text: btnLabel.text
        signal clicked()
        width: 28; height: 28; radius: 4
        color: btnMa.containsMouse ? theme.surfaceAlt : theme.surface
        border.color: theme.border; border.width: 1
        Text {
            id: btnLabel; anchors.centerIn: parent
            font.family: settings.fontFamily; font.pixelSize: 14; color: theme.text
        }
        MouseArea {
            id: btnMa; anchors.fill: parent
            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
    }
}
