import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Slate 1.0

Rectangle {
    id: statusBarRoot
    color: theme.background
    border.color: theme.border
    border.width: 1

    property EditorBuffer currentBuffer: null
    property LspClient lspClient: null
    property string projectName: ""
    property bool isGitRepo: false

    RowLayout {
        anchors { fill: parent; leftMargin: 12; rightMargin: 12 }
        spacing: 16

        // Git branch indicator
        Text {
            visible: isGitRepo
            text: "⎇ " + projectName
            font.family: settings.fontFamily; font.pixelSize: 10
            color: theme.accent
        }

        // Diagnostics summary
        Text {
            visible: lspClient && lspClient.running && lspClient.diagnostics.length > 0
            text: {
                if (!lspClient) return ""
                var errors = 0, warnings = 0
                for (var i = 0; i < lspClient.diagnostics.length; i++) {
                    var sev = lspClient.diagnostics[i].severity
                    if (sev === 1) errors++
                    else if (sev === 2) warnings++
                }
                var parts = []
                if (errors > 0) parts.push("✕ " + errors)
                if (warnings > 0) parts.push("⚠ " + warnings)
                return parts.join("  ")
            }
            font.family: settings.fontFamily; font.pixelSize: 10
            color: {
                if (!lspClient) return theme.textDim
                for (var i = 0; i < lspClient.diagnostics.length; i++) {
                    if (lspClient.diagnostics[i].severity === 1) return theme.error
                }
                return theme.warning
            }
        }

        Item { Layout.fillWidth: true }

        // Cursor position
        Text {
            visible: currentBuffer !== null
            text: currentBuffer ? ("Ln " + currentBuffer.cursorLine + ", Col " + currentBuffer.cursorColumn) : ""
            font.family: settings.fontFamily; font.pixelSize: 10
            color: theme.textDim
        }

        // Language
        Text {
            visible: currentBuffer !== null
            text: currentBuffer ? currentBuffer.language : ""
            font.family: settings.fontFamily; font.pixelSize: 10
            color: theme.accent
        }

        // LSP status
        Rectangle {
            width: lspStatusText.implicitWidth + 12; height: 16; radius: 3
            color: lspClient && lspClient.running ? theme.surface : theme.surface
            border.color: lspClient && lspClient.running ? theme.borderFocus : theme.border
            border.width: 1
            Text {
                id: lspStatusText
                anchors.centerIn: parent
                text: lspClient && lspClient.running ? ("LSP: " + lspClient.serverName) : "LSP: off"
                font.family: settings.fontFamily; font.pixelSize: 9
                color: lspClient && lspClient.running ? theme.accent : theme.textMuted
            }
        }

        // Encoding
        Text {
            text: "UTF-8"
            font.family: settings.fontFamily; font.pixelSize: 10
            color: theme.textMuted
        }
    }
}
