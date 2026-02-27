import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Slate 1.0

Item {
    id: editorArea

    property EditorBuffer currentBuffer: null
    property LspClient lspClient: lsp
    property var buffers: []
    property int currentIndex: -1

    signal fileOpened()

    LspClient { id: lsp }

    // Timer for LSP change notifications (debounced)
    Timer {
        id: lspChangeTimer
        interval: 500; repeat: false
        onTriggered: {
            if (currentBuffer && lsp.running) {
                lsp.didChange("file://" + currentBuffer.filePath, currentBuffer.content)
                // Request semantic tokens after change
                semanticTimer.restart()
            }
        }
    }

    // Timer for semantic tokens (slightly longer debounce — heavier request)
    Timer {
        id: semanticTimer
        interval: 800; repeat: false
        onTriggered: {
            if (currentBuffer && lsp.running && lsp.hasSemanticTokens()) {
                lsp.requestSemanticTokens("file://" + currentBuffer.filePath)
            }
        }
    }

    // Timer for completion requests
    Timer {
        id: completionTimer
        interval: 300; repeat: false
        onTriggered: {
            if (currentBuffer && lsp.running) {
                lsp.requestCompletion(
                    "file://" + currentBuffer.filePath,
                    currentBuffer.cursorLine - 1,
                    currentBuffer.cursorColumn - 1
                )
            }
        }
    }

    function openFile(path) {
        // Check if already open
        for (var i = 0; i < buffers.length; i++) {
            if (buffers[i].filePath === path) {
                currentIndex = i
                currentBuffer = buffers[i]
                textEdit.text = currentBuffer.content
                return
            }
        }
        var buf = bufComp.createObject(editorArea)
        if (!buf.loadFile(path)) {
            buf.destroy()
            return
        }
        buffers.push(buf)
        currentIndex = buffers.length - 1
        currentBuffer = buf
        textEdit.text = currentBuffer.content
        tabRepeater.model = buffers.length // force refresh
        fileOpened()

        // Start LSP if available
        tryStartLsp(buf)

        // Notify LSP
        if (lsp.running) {
            lsp.didOpen("file://" + buf.filePath, buf.languageId(), buf.content)
            // Request semantic tokens after a short delay to let server index
            semanticTimer.restart()
        }
    }

    function newFile() {
        var buf = bufComp.createObject(editorArea)
        buffers.push(buf)
        currentIndex = buffers.length - 1
        currentBuffer = buf
        textEdit.text = ""
        tabRepeater.model = buffers.length
        fileOpened()
    }

    function closeCurrentTab() {
        if (currentIndex < 0 || buffers.length === 0) return
        if (lsp.running && currentBuffer.filePath) {
            lsp.didClose("file://" + currentBuffer.filePath)
        }
        var buf = buffers[currentIndex]
        buffers.splice(currentIndex, 1)
        buf.destroy()
        if (buffers.length === 0) {
            currentIndex = -1
            currentBuffer = null
            textEdit.text = ""
        } else {
            currentIndex = Math.min(currentIndex, buffers.length - 1)
            currentBuffer = buffers[currentIndex]
            textEdit.text = currentBuffer.content
        }
        tabRepeater.model = buffers.length
    }

    function nextTab() {
        if (buffers.length <= 1) return
        currentIndex = (currentIndex + 1) % buffers.length
        switchToTab(currentIndex)
    }

    function prevTab() {
        if (buffers.length <= 1) return
        currentIndex = (currentIndex - 1 + buffers.length) % buffers.length
        switchToTab(currentIndex)
    }

    function switchToTab(idx) {
        currentIndex = idx
        currentBuffer = buffers[idx]
        textEdit.text = currentBuffer.content
        tabRepeater.model = buffers.length
    }

    function tryStartLsp(buf) {
        if (!settings.lspEnabled || !settings.lspAutostart) return
        var langId = buf.languageId()
        var cmd = settings.lspServerFor(langId)
        if (cmd && !lsp.running) {
            var root = projectManager.projectRoot || buf.filePath.substring(0, buf.filePath.lastIndexOf("/"))
            var args = settings.lspArgsFor(langId)
            lsp.startServer(cmd, args, root)
        }
    }

    function restartLsp() {
        if (lsp.running) lsp.shutdown()
        if (currentBuffer) tryStartLsp(currentBuffer)
    }

    // When LSP connects, send didOpen for current file and request semantic tokens
    Connections {
        target: lsp
        function onRunningChanged() {
            if (lsp.running && currentBuffer && currentBuffer.filePath) {
                lsp.didOpen("file://" + currentBuffer.filePath, currentBuffer.languageId(), currentBuffer.content)
                semanticTimer.restart()
            }
        }
    }

    Component {
        id: bufComp
        EditorBuffer {}
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Tab bar
        Rectangle {
            Layout.fillWidth: true; height: buffers.length > 0 ? 34 : 0
            color: theme.background; visible: height > 0
            clip: true

            Row {
                id: tabRow
                anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
                Repeater {
                    id: tabRepeater
                    model: buffers.length
                    delegate: Rectangle {
                        width: tabLabel.implicitWidth + 52
                        height: 34
                        color: index === currentIndex ? theme.backgroundAlt : tabHover.containsMouse ? theme.surface : theme.background
                        border.color: index === currentIndex ? theme.border : "transparent"
                        border.width: 1

                        Rectangle {
                            width: parent.width; height: 2
                            color: index === currentIndex ? theme.accent : "transparent"
                        }

                        RowLayout {
                            anchors { fill: parent; leftMargin: 12; rightMargin: 8 }
                            spacing: 6
                            Text {
                                id: tabLabel
                                text: {
                                    var b = buffers[index]
                                    return b ? ((b.modified ? "• " : "") + (b.fileName || "untitled")) : ""
                                }
                                font.family: settings.fontFamily; font.pixelSize: 11
                                color: index === currentIndex ? theme.text : theme.textMuted
                                elide: Text.ElideRight; Layout.fillWidth: true
                            }
                            Rectangle {
                                width: 18; height: 18; radius: 3
                                color: closeHover.containsMouse ? theme.error + "33" : "transparent"
                                Text {
                                    anchors.centerIn: parent; text: "×"
                                    font.pixelSize: 14; color: closeHover.containsMouse ? theme.error : theme.textMuted
                                }
                                HoverHandler { id: closeHover }
                                TapHandler {
                                    onTapped: {
                                        currentIndex = index
                                        currentBuffer = buffers[index]
                                        closeCurrentTab()
                                    }
                                }
                            }
                        }

                        HoverHandler { id: tabHover }
                        TapHandler { onTapped: switchToTab(index) }
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: theme.border }

        // Editor body
        Item {
            Layout.fillWidth: true; Layout.fillHeight: true

            // Line numbers gutter
            Flickable {
                id: lineNumberFlick
                anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
                width: 52
                contentY: editorFlick.contentY
                clip: true
                interactive: false

                Rectangle {
                    width: 52; height: Math.max(lineNumCol.implicitHeight, editorFlick.height)
                    color: theme.background

                    Column {
                        id: lineNumCol
                        anchors { right: parent.right; rightMargin: 10; top: parent.top; topMargin: 8 }
                        Repeater {
                            model: currentBuffer ? currentBuffer.lineCount : 1
                            delegate: Text {
                                text: (index + 1).toString()
                                font.family: settings.fontFamily; font.pixelSize: settings.fontSize
                                color: (index + 1) === (currentBuffer ? currentBuffer.cursorLine : 1) ? theme.accent : theme.textMuted
                                height: 20
                                horizontalAlignment: Text.AlignRight
                                width: 32
                            }
                        }
                    }
                }
            }

            Rectangle {
                x: 52
                width: 1
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                color: theme.border
            }

            // Text area
            Flickable {
                id: editorFlick
                anchors { left: parent.left; leftMargin: 53; top: parent.top; right: parent.right; bottom: parent.bottom }
                contentWidth: textEdit.paintedWidth + 24
                contentHeight: textEdit.paintedHeight + 24
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.AutoFlickIfNeeded

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                    contentItem: Rectangle { implicitWidth: 6; radius: 3; color: theme.borderFocus }
                    background: Item {}
                }
                ScrollBar.horizontal: ScrollBar {
                    policy: ScrollBar.AsNeeded
                    contentItem: Rectangle { implicitHeight: 6; radius: 3; color: theme.borderFocus }
                    background: Item {}
                }

                TextEdit {
                    id: textEdit
                    width: Math.max(editorFlick.width - 24, implicitWidth)
                    padding: 8
                    topPadding: 8
                    font.family: settings.fontFamily
                    font.pixelSize: settings.fontSize
                    color: theme.text
                    selectionColor: theme.selection
                    selectedTextColor: theme.text
                    wrapMode: settings.wordWrap ? TextEdit.Wrap : TextEdit.NoWrap
                    selectByMouse: true
                    tabStopDistance: settings.tabWidth * fontMetrics.advanceWidth(' ')

                    FontMetrics { id: fontMetrics; font: textEdit.font }

                    // Syntax highlighter
                    SyntaxHighlighter {
                        id: highlighter
                        document: textEdit.textDocument
                        language: currentBuffer ? currentBuffer.languageId() : ""
                        colorKeyword: theme.accent
                        colorType: theme.warning || "#79C0FF"
                        colorString: theme.success || "#A5D6FF"
                        colorComment: theme.textMuted
                        colorNumber: theme.warning || "#F2CC60"
                        colorFunction: theme.textDim
                        colorPreprocessor: theme.error || "#FFA657"
                        colorOperator: theme.accent
                    }

                    // Update highlighter when buffer changes
                    Connections {
                        target: editorArea
                        function onCurrentBufferChanged() {
                            if (currentBuffer)
                                highlighter.language = currentBuffer.languageId()
                        }
                    }

                    // Update diagnostics in highlighter
                    Connections {
                        target: lsp
                        function onDiagnosticsChanged() {
                            var diags = []
                            for (var i = 0; i < lsp.diagnostics.length; i++)
                                diags.push(lsp.diagnostics[i])
                            highlighter.setDiagnostics(diags)
                        }
                        function onSemanticTokensChanged() {
                            highlighter.setSemanticTokens(lsp.semanticTokens)
                        }
                    }

                    onTextChanged: {
                        if (currentBuffer && text !== currentBuffer.content) {
                            currentBuffer.content = text
                            lspChangeTimer.restart()
                        }
                    }

                    onCursorPositionChanged: {
                        if (!currentBuffer) return
                        // Calculate line and column from cursor position
                        var textBefore = text.substring(0, cursorPosition)
                        var lines = textBefore.split('\n')
                        currentBuffer.cursorLine = lines.length
                        currentBuffer.cursorColumn = lines[lines.length - 1].length + 1
                    }

                    Keys.onPressed: function(event) {
                        // Trigger completion on Ctrl+Space
                        if (event.key === Qt.Key_Space && (event.modifiers & Qt.ControlModifier)) {
                            completionTimer.restart()
                            event.accepted = true
                        }
                        // Accept completion on Tab/Enter if popup visible
                        if (completionPopup.visible && (event.key === Qt.Key_Tab || event.key === Qt.Key_Return)) {
                            var item = completionList.currentItem
                            if (item) {
                                // Insert completion text
                                // Simple: just insert the label
                                var lbl = lsp.completions[completionList.currentIndex]["label"]
                                if (lbl) {
                                    textEdit.insert(textEdit.cursorPosition, lbl)
                                }
                            }
                            completionPopup.visible = false
                            event.accepted = true
                        }
                        if (completionPopup.visible && event.key === Qt.Key_Escape) {
                            completionPopup.visible = false
                            event.accepted = true
                        }
                        if (completionPopup.visible && event.key === Qt.Key_Down) {
                            completionList.currentIndex = Math.min(completionList.currentIndex + 1, completionList.count - 1)
                            event.accepted = true
                        }
                        if (completionPopup.visible && event.key === Qt.Key_Up) {
                            completionList.currentIndex = Math.max(completionList.currentIndex - 1, 0)
                            event.accepted = true
                        }
                    }

                    // Right-click context menu
                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: editorContextMenu.popup()
                    }
                }
            }

            // Completion popup
            Rectangle {
                id: completionPopup
                visible: lsp.completions.length > 0
                x: {
                    if (!currentBuffer) return 53
                    var col = currentBuffer.cursorColumn - 1
                    return 53 + 8 + col * fontMetrics.advanceWidth('m') - editorFlick.contentX
                }
                y: {
                    if (!currentBuffer) return 0
                    var line = currentBuffer.cursorLine
                    return 8 + line * fontMetrics.height - editorFlick.contentY
                }
                width: 320; height: Math.min(completionList.count * 26, 200)
                color: theme.backgroundAlt; border.color: theme.borderFocus; border.width: 1; radius: 4
                z: 100

                ListView {
                    id: completionList
                    anchors { fill: parent; margins: 2 }
                    model: lsp.completions
                    clip: true
                    currentIndex: 0
                    delegate: Rectangle {
                        width: completionList.width; height: 26
                        color: index === completionList.currentIndex ? theme.hover : "transparent"
                        RowLayout {
                            anchors { fill: parent; leftMargin: 8; rightMargin: 8 }
                            spacing: 6
                            Text {
                                text: {
                                    var kind = modelData.kind || 0
                                    var kinds = {1:"txt",2:"fn",3:"fn",4:"",5:"field",6:"var",
                                                 7:"class",8:"iface",9:"mod",10:"prop",
                                                 13:"enum",14:"kw",15:"snip",21:"const",22:"struct"}
                                    return kinds[kind] || "·"
                                }
                                font.family: settings.fontFamily; font.pixelSize: 9
                                color: theme.accent; Layout.preferredWidth: 36
                            }
                            Text {
                                text: modelData.label || ""
                                font.family: settings.fontFamily; font.pixelSize: 12
                                color: theme.text; elide: Text.ElideRight; Layout.fillWidth: true
                            }
                        }
                    }
                }
            }

            // Hover tooltip
            Rectangle {
                id: hoverTooltip
                visible: false
                x: 200; y: 100
                width: hoverText.implicitWidth + 20
                height: hoverText.implicitHeight + 16
                color: theme.backgroundAlt; border.color: theme.borderFocus; border.width: 1; radius: 4
                z: 100

                Text {
                    id: hoverText
                    anchors { fill: parent; margins: 8 }
                    font.family: settings.fontFamily; font.pixelSize: 11
                    color: theme.text; wrapMode: Text.Wrap
                }

                Timer {
                    id: hoverHideTimer
                    interval: 5000; onTriggered: hoverTooltip.visible = false
                }
            }

            Connections {
                target: lsp
                function onHoverResult(text) {
                    if (text) {
                        hoverText.text = text
                        hoverTooltip.visible = true
                        hoverHideTimer.restart()
                    }
                }
                function onDefinitionResult(uri, line, character) {
                    if (uri) {
                        var path = uri.replace("file://", "")
                        openFile(path)
                        // TODO: scroll to line
                    }
                }
            }

            // Empty state
            Item {
                anchors.fill: parent
                visible: !currentBuffer
                Text {
                    anchors.centerIn: parent
                    text: "Ctrl+N new file · Ctrl+O open file"
                    font.family: settings.fontFamily; font.pixelSize: 13; color: theme.textMuted
                }
            }
        }
    }

    // Editor context menu
    Menu {
        id: editorContextMenu
        background: Rectangle {
            implicitWidth: 220; color: theme.backgroundAlt
            border.color: theme.borderFocus; border.width: 1; radius: 6
        }

        MenuItem {
            text: "Cut                  Ctrl+X"
            contentItem: Text { text: parent.text; font.family: settings.fontFamily; font.pixelSize: 12; color: theme.text }
            background: Rectangle { color: parent.highlighted ? theme.hover : "transparent"; radius: 4 }
            onTriggered: textEdit.cut()
        }
        MenuItem {
            text: "Copy                 Ctrl+C"
            contentItem: Text { text: parent.text; font.family: settings.fontFamily; font.pixelSize: 12; color: theme.text }
            background: Rectangle { color: parent.highlighted ? theme.hover : "transparent"; radius: 4 }
            onTriggered: textEdit.copy()
        }
        MenuItem {
            text: "Paste                Ctrl+V"
            contentItem: Text { text: parent.text; font.family: settings.fontFamily; font.pixelSize: 12; color: theme.text }
            background: Rectangle { color: parent.highlighted ? theme.hover : "transparent"; radius: 4 }
            onTriggered: textEdit.paste()
        }
        MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: theme.border } }
        MenuItem {
            text: "Select All           Ctrl+A"
            contentItem: Text { text: parent.text; font.family: settings.fontFamily; font.pixelSize: 12; color: theme.text }
            background: Rectangle { color: parent.highlighted ? theme.hover : "transparent"; radius: 4 }
            onTriggered: textEdit.selectAll()
        }
        MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: theme.border } }
        MenuItem {
            text: "Go to Definition     F12"
            enabled: lsp.running
            contentItem: Text {
                text: parent.text; font.family: "Courier New"; font.pixelSize: 12
                color: parent.enabled ? theme.text : theme.textMuted
            }
            background: Rectangle { color: parent.highlighted ? theme.hover : "transparent"; radius: 4 }
            onTriggered: {
                if (currentBuffer && lsp.running) {
                    lsp.requestDefinition(
                        "file://" + currentBuffer.filePath,
                        currentBuffer.cursorLine - 1,
                        currentBuffer.cursorColumn - 1
                    )
                }
            }
        }
        MenuItem {
            text: "Show Hover           Ctrl+H"
            enabled: lsp.running
            contentItem: Text {
                text: parent.text; font.family: "Courier New"; font.pixelSize: 12
                color: parent.enabled ? theme.text : theme.textMuted
            }
            background: Rectangle { color: parent.highlighted ? theme.hover : "transparent"; radius: 4 }
            onTriggered: {
                if (currentBuffer && lsp.running) {
                    lsp.requestHover(
                        "file://" + currentBuffer.filePath,
                        currentBuffer.cursorLine - 1,
                        currentBuffer.cursorColumn - 1
                    )
                }
            }
        }
        MenuItem {
            text: "Trigger Completion   Ctrl+Space"
            enabled: lsp.running
            contentItem: Text {
                text: parent.text; font.family: "Courier New"; font.pixelSize: 12
                color: parent.enabled ? theme.text : theme.textMuted
            }
            background: Rectangle { color: parent.highlighted ? theme.hover : "transparent"; radius: 4 }
            onTriggered: completionTimer.restart()
        }
        MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: theme.border } }
        MenuItem {
            text: "Save                 Ctrl+S"
            contentItem: Text { text: parent.text; font.family: settings.fontFamily; font.pixelSize: 12; color: theme.text }
            background: Rectangle { color: parent.highlighted ? theme.hover : "transparent"; radius: 4 }
            onTriggered: { if (currentBuffer) currentBuffer.saveFile() }
        }
        MenuItem {
            text: "Close Tab            Ctrl+W"
            contentItem: Text { text: parent.text; font.family: settings.fontFamily; font.pixelSize: 12; color: theme.text }
            background: Rectangle { color: parent.highlighted ? theme.hover : "transparent"; radius: 4 }
            onTriggered: closeCurrentTab()
        }
    }
}
