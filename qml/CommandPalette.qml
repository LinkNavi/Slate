import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: palette
    height: searchField.height + resultsList.contentHeight + 16
    radius: 8
    color: theme.backgroundAlt
    border.color: theme.borderFocus
    border.width: 1
    z: 1000

    signal commandSelected(string cmd)
    signal closed()

    property var commands: [
        { label: "New File",              cmd: "new_file",         shortcut: "Ctrl+N" },
        { label: "Open File",             cmd: "open_file",        shortcut: "Ctrl+O" },
        { label: "Open Folder",           cmd: "open_folder",      shortcut: "Ctrl+K" },
        { label: "Save",                  cmd: "save",             shortcut: "Ctrl+S" },
        { label: "Save As",              cmd: "save_as",          shortcut: "Ctrl+Shift+S" },
        { label: "Close Tab",            cmd: "close_tab",        shortcut: "Ctrl+W" },
        { label: "Toggle Explorer",      cmd: "toggle_explorer",  shortcut: "Ctrl+B" },
        { label: "Settings",             cmd: "settings",         shortcut: "Ctrl+," },
        { label: "LSP: Restart Server",  cmd: "lsp_restart",      shortcut: "" },
        { label: "LSP: Stop Server",     cmd: "lsp_stop",         shortcut: "" },
        { label: "LSP: Show Hover",      cmd: "lsp_hover",        shortcut: "Ctrl+H" },
        { label: "LSP: Go to Definition",cmd: "lsp_definition",   shortcut: "F12" },
        { label: "LSP: Trigger Completion",cmd: "lsp_completion", shortcut: "Ctrl+Space" },
        { label: "Theme: Reload Matugen", cmd: "reload_matugen",  shortcut: "" },
    ]

    property var filtered: {
        var q = searchField.text.toLowerCase()
        if (!q) return commands
        return commands.filter(function(c) {
            return c.label.toLowerCase().indexOf(q) >= 0
        })
    }

    Column {
        anchors { fill: parent; margins: 8 }
        spacing: 4

        TextField {
            id: searchField
            width: parent.width
            placeholderText: "Type a command..."
            font.family: settings.fontFamily; font.pixelSize: 13
            color: theme.text
            placeholderTextColor: theme.textMuted
            background: Rectangle { color: theme.surface; radius: 4; border.color: theme.border; border.width: 1 }
            padding: 8

            Component.onCompleted: forceActiveFocus()

            Keys.onEscapePressed: palette.closed()
            Keys.onReturnPressed: {
                if (palette.filtered.length > 0) {
                    palette.commandSelected(palette.filtered[resultsList.currentIndex || 0].cmd)
                }
            }
            Keys.onDownPressed: resultsList.currentIndex = Math.min(resultsList.currentIndex + 1, resultsList.count - 1)
            Keys.onUpPressed: resultsList.currentIndex = Math.max(resultsList.currentIndex - 1, 0)
        }

        ListView {
            id: resultsList
            width: parent.width
            height: Math.min(count * 32, 250)
            model: palette.filtered
            clip: true
            currentIndex: 0

            delegate: Rectangle {
                width: resultsList.width; height: 32; radius: 4
                color: index === resultsList.currentIndex ? theme.hover : "transparent"

                RowLayout {
                    anchors { fill: parent; leftMargin: 10; rightMargin: 10 }
                    spacing: 8
                    Text {
                        text: modelData.label
                        font.family: settings.fontFamily; font.pixelSize: 12
                        color: theme.text; Layout.fillWidth: true
                    }
                    Text {
                        text: modelData.shortcut
                        font.family: settings.fontFamily; font.pixelSize: 10
                        color: theme.textMuted
                    }
                }

                TapHandler {
                    onTapped: palette.commandSelected(modelData.cmd)
                }
            }
        }
    }
}
