import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Slate 1.0

Window {
    id: root
    width: 1280
    height: 800
    visible: true
    title: {
        var t = "Slate"
        if (editorStack.currentBuffer) {
            t = editorStack.currentBuffer.fileName + (editorStack.currentBuffer.modified ? " •" : "") + " — Slate"
            if (projectManager.projectName)
                t += " [" + projectManager.projectName + "]"
        } else if (projectManager.projectName) {
            t += " [" + projectManager.projectName + "]"
        }
        return t
    }
    color: theme.background

    property bool showExplorer: true
    property bool showWelcome: true
    property bool showCommandPalette: false
    property bool showSettings: false

    // Background grid
    Canvas {
        id: gridCanvas
        anchors.fill: parent
        opacity: 0.04
        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.strokeStyle = theme.accent
            ctx.lineWidth = 0.5
            var spacing = 40
            for (var x = 0; x <= width; x += spacing) {
                ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, height); ctx.stroke()
            }
            for (var y = 0; y <= height; y += spacing) {
                ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(width, y); ctx.stroke()
            }
        }
        Connections {
            target: theme
            function onThemeChanged() { gridCanvas.requestPaint() }
        }
    }

    // Keybinds
    Shortcut { sequence: "Ctrl+B"; onActivated: root.showExplorer = !root.showExplorer }
    Shortcut {
        sequence: "Ctrl+N"
        onActivated: {
            editorStack.newFile()
            root.showWelcome = false; root.showSettings = false
        }
    }
    Shortcut {
        sequence: "Ctrl+O"
        onActivated: {
            var path = dialogHelper.openFile()
            if (path) { editorStack.openFile(path); root.showWelcome = false; root.showSettings = false }
        }
    }
    Shortcut {
        sequence: "Ctrl+K"
        onActivated: {
            var path = dialogHelper.openFolder()
            if (path) {
                projectManager.openDirectory(path)
                fileSystemModel.rootPath = projectManager.projectRoot || path
                root.showExplorer = true; root.showWelcome = false; root.showSettings = false
            }
        }
    }
    Shortcut { sequence: "Ctrl+S"; onActivated: { if (editorStack.currentBuffer) editorStack.currentBuffer.saveFile() } }
    Shortcut { sequence: "Ctrl+W"; onActivated: editorStack.closeCurrentTab() }
    Shortcut { sequence: "Ctrl+P"; onActivated: root.showCommandPalette = !root.showCommandPalette }
    Shortcut {
        sequence: "Ctrl+Shift+S"
        onActivated: {
            if (editorStack.currentBuffer) {
                var path = dialogHelper.saveFile()
                if (path) editorStack.currentBuffer.saveFileAs(path)
            }
        }
    }
    Shortcut { sequence: "Ctrl+Tab"; onActivated: editorStack.nextTab() }
    Shortcut { sequence: "Ctrl+Shift+Tab"; onActivated: editorStack.prevTab() }
    Shortcut { sequence: "Ctrl+,"; onActivated: root.showSettings = !root.showSettings }
    Shortcut { sequence: "Escape"; onActivated: {
        if (root.showCommandPalette) root.showCommandPalette = false
        else if (root.showSettings) root.showSettings = false
    }}

    // Main layout
    RowLayout {
        anchors { fill: parent; bottomMargin: 26 }
        spacing: 0

        // File Explorer sidebar
        Rectangle {
            id: explorerPanel
            width: root.showExplorer && !root.showWelcome ? 240 : 0
            Layout.fillHeight: true
            color: theme.backgroundAlt
            border.color: theme.border
            border.width: width > 0 ? 1 : 0
            clip: true
            visible: width > 0

            Behavior on width { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }

            FileExplorer {
                id: fileExplorer
                anchors.fill: parent
                onFileSelected: function(path) {
                    editorStack.openFile(path)
                    root.showWelcome = false; root.showSettings = false
                }
            }
        }

        // Editor area
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            WelcomeScreen {
                id: welcomeScreen
                anchors.fill: parent
                visible: root.showWelcome && !root.showSettings
                onOpenFile: {
                    var p = dialogHelper.openFile()
                    if (p) { editorStack.openFile(p); root.showWelcome = false }
                }
                onOpenFolder: {
                    var d = dialogHelper.openFolder()
                    if (d) {
                        projectManager.openDirectory(d)
                        fileSystemModel.rootPath = projectManager.projectRoot || d
                        root.showExplorer = true; root.showWelcome = false
                    }
                }
                onNewFile: {
                    editorStack.newFile()
                    root.showWelcome = false
                }
                onProjectOpened: function(path) {
                    projectManager.openDirectory(path)
                    fileSystemModel.rootPath = projectManager.projectRoot || path
                    root.showExplorer = true; root.showWelcome = false
                }
            }

            EditorArea {
                id: editorStack
                anchors.fill: parent
                visible: !root.showWelcome && !root.showSettings
                onFileOpened: root.showWelcome = false
            }

            SettingsPanel {
                anchors.fill: parent
                visible: root.showSettings
                onClosed: root.showSettings = false
            }
        }
    }

    // Status bar
    StatusBar {
        id: statusBar
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        height: 26
        currentBuffer: editorStack.currentBuffer
        lspClient: editorStack.lspClient
        projectName: projectManager.projectName
        isGitRepo: projectManager.isGitRepo
    }

    // Toasts
    ToastManager {
        id: toasts
        anchors.fill: parent
    }

    // LSP toast connections
    Connections {
        target: editorStack.lspClient
        function onRunningChanged() {
            if (editorStack.lspClient.running)
                toasts.show("LSP connected: " + editorStack.lspClient.serverName, "success")
            else
                toasts.show("LSP disconnected", "warning")
        }
        function onDiagnosticsChanged() {
            var diags = editorStack.lspClient.diagnostics
            var errors = 0
            for (var i = 0; i < diags.length; i++) {
                if (diags[i].severity === 1) errors++
            }
            if (errors > 0)
                toasts.show(errors + " error" + (errors > 1 ? "s" : "") + " found", "error")
        }
    }

    // Command palette overlay
    CommandPalette {
        id: cmdPalette
        visible: root.showCommandPalette
        anchors.horizontalCenter: parent.horizontalCenter
        y: 80; width: 500

        onCommandSelected: function(cmd) {
            root.showCommandPalette = false
            executeCommand(cmd)
        }
        onClosed: root.showCommandPalette = false
    }

    function executeCommand(cmd) {
        switch(cmd) {
            case "new_file":
                editorStack.newFile(); root.showWelcome = false; root.showSettings = false; break
            case "open_file": {
                var p = dialogHelper.openFile()
                if (p) { editorStack.openFile(p); root.showWelcome = false; root.showSettings = false }
                break
            }
            case "open_folder": {
                var d = dialogHelper.openFolder()
                if (d) {
                    projectManager.openDirectory(d)
                    fileSystemModel.rootPath = projectManager.projectRoot || d
                    root.showExplorer = true; root.showWelcome = false; root.showSettings = false
                }
                break
            }
            case "save":
                if (editorStack.currentBuffer) editorStack.currentBuffer.saveFile(); break
            case "save_as": {
                if (editorStack.currentBuffer) {
                    var s = dialogHelper.saveFile()
                    if (s) editorStack.currentBuffer.saveFileAs(s)
                }
                break
            }
            case "close_tab": editorStack.closeCurrentTab(); break
            case "toggle_explorer": root.showExplorer = !root.showExplorer; break
            case "settings": root.showSettings = true; break
            case "lsp_restart":
                if (editorStack.lspClient.running) editorStack.lspClient.shutdown()
                if (editorStack.currentBuffer) editorStack.restartLsp()
                break
            case "lsp_stop":
                editorStack.lspClient.shutdown(); break
            case "lsp_hover":
                if (editorStack.currentBuffer && editorStack.lspClient.running) {
                    editorStack.lspClient.requestHover(
                        "file://" + editorStack.currentBuffer.filePath,
                        editorStack.currentBuffer.cursorLine - 1,
                        editorStack.currentBuffer.cursorColumn - 1)
                }
                break
            case "lsp_definition":
                if (editorStack.currentBuffer && editorStack.lspClient.running) {
                    editorStack.lspClient.requestDefinition(
                        "file://" + editorStack.currentBuffer.filePath,
                        editorStack.currentBuffer.cursorLine - 1,
                        editorStack.currentBuffer.cursorColumn - 1)
                }
                break
            case "lsp_completion":
                if (editorStack.currentBuffer && editorStack.lspClient.running) {
                    editorStack.lspClient.requestCompletion(
                        "file://" + editorStack.currentBuffer.filePath,
                        editorStack.currentBuffer.cursorLine - 1,
                        editorStack.currentBuffer.cursorColumn - 1)
                }
                break
            case "reload_matugen":
                theme.reloadMatugen()
                toasts.show("Matugen colors reloaded", "info")
                break
        }
    }

    // Scanlines overlay
    Canvas {
        anchors.fill: parent
        opacity: 0.02
        onPaint: {
            var ctx = getContext("2d")
            for (var y = 0; y < height; y += 3) {
                ctx.fillStyle = "#000000"
                ctx.fillRect(0, y, width, 1)
            }
        }
    }
}
