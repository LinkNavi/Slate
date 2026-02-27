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

    readonly property real gutterWidth: Math.max(48, editorArea.width * 0.05)
    readonly property real tabBarHeight: editorArea.height * 0.045
    readonly property real charWidth: fontMetrics.advanceWidth('m')

    signal fileOpened()

    LspClient { id: lsp }

    Timer {
        id: lspChangeTimer
        interval: 500; repeat: false
        onTriggered: {
            if (currentBuffer && lsp.running) {
                lsp.didChange("file://" + currentBuffer.filePath, currentBuffer.content)
                semanticTimer.restart()
                completionTimer.restart()
            }
        }
    }

    Timer {
        id: semanticTimer
        interval: 800; repeat: false
        onTriggered: {
            if (currentBuffer && lsp.running && lsp.hasSemanticTokens())
                lsp.requestSemanticTokens("file://" + currentBuffer.filePath)
        }
    }

    Timer {
        id: completionTimer
        interval: 200; repeat: false
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
        for (var i = 0; i < buffers.length; i++) {
            if (buffers[i].filePath === path) {
                currentIndex = i
                currentBuffer = buffers[i]
                textEdit.text = currentBuffer.content
                return
            }
        }
        var buf = bufComp.createObject(editorArea)
        if (!buf.loadFile(path)) { buf.destroy(); return }
        buffers.push(buf)
        currentIndex = buffers.length - 1
        currentBuffer = buf
        textEdit.text = currentBuffer.content
        tabRepeater.model = buffers.length
        fileOpened()
        tryStartLsp(buf)
        if (lsp.running) {
            lsp.didOpen("file://" + buf.filePath, buf.languageId(), buf.content)
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
        if (lsp.running && currentBuffer.filePath)
            lsp.didClose("file://" + currentBuffer.filePath)
        var buf = buffers[currentIndex]
        buffers.splice(currentIndex, 1)
        buf.destroy()
        if (buffers.length === 0) {
            currentIndex = -1; currentBuffer = null; textEdit.text = ""
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
            lsp.startServer(cmd, settings.lspArgsFor(langId), root)
        }
    }

    function restartLsp() {
        if (lsp.running) lsp.shutdown()
        if (currentBuffer) tryStartLsp(currentBuffer)
    }

    Connections {
        target: lsp
        function onRunningChanged() {
            if (lsp.running && currentBuffer && currentBuffer.filePath) {
                lsp.didOpen("file://" + currentBuffer.filePath, currentBuffer.languageId(), currentBuffer.content)
                semanticTimer.restart()
            }
        }
    }

    Component { id: bufComp; EditorBuffer {} }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Tab bar
        Rectangle {
            Layout.fillWidth: true
            height: buffers.length > 0 ? tabBarHeight : 0
            color: theme.background
            visible: height > 0
            clip: true

            Row {
                anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
                Repeater {
                    id: tabRepeater
                    model: buffers.length
                    delegate: Rectangle {
                        width: tabLabel.implicitWidth + editorArea.width * 0.04
                        height: tabBarHeight
                        color: index === currentIndex ? theme.backgroundAlt : tabHover.containsMouse ? theme.surface : theme.background
                        border.color: index === currentIndex ? theme.border : "transparent"
                        border.width: 1

                        Rectangle {
                            width: parent.width; height: parent.height * 0.06
                            color: index === currentIndex ? theme.accent : "transparent"
                        }

                        RowLayout {
                            anchors { fill: parent; leftMargin: parent.width * 0.08; rightMargin: parent.width * 0.05 }
                            spacing: editorArea.width * 0.004
                            Text {
                                id: tabLabel
                                text: {
                                    var b = buffers[index]
                                    return b ? ((b.modified ? "• " : "") + (b.fileName || "untitled")) : ""
                                }
                                font.family: settings.fontFamily
                                font.pixelSize: tabBarHeight * 0.32
                                color: index === currentIndex ? theme.text : theme.textMuted
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            Rectangle {
                                width: tabBarHeight * 0.52; height: tabBarHeight * 0.52; radius: 3
                                color: closeHover.containsMouse ? theme.error + "33" : "transparent"
                                Text {
                                    anchors.centerIn: parent; text: "×"
                                    font.pixelSize: tabBarHeight * 0.4
                                    color: closeHover.containsMouse ? theme.error : theme.textMuted
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
            id: editorBody
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Gutter — strictly contained, never bleeds outside editorBody
            Item {
                id: gutterContainer
                anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
                width: gutterWidth
                clip: true
                z: 0

                Rectangle { anchors.fill: parent; color: theme.background }

                Flickable {
                    id: lineNumberFlick
                    anchors.fill: parent
                    contentY: editorFlick.contentY
                    interactive: false
                    clip: true

                    Column {
                        anchors {
                            right: parent.right
                            rightMargin: gutterWidth * 0.12
                            top: parent.top
                            topMargin: editorArea.height * 0.01
                        }
                        width: gutterWidth

                        Repeater {
                            model: currentBuffer ? currentBuffer.lineCount : 1
                            delegate: Text {
                                text: (index + 1).toString()
                                font.family: settings.fontFamily
                                font.pixelSize: settings.fontSize
                                color: (index + 1) === (currentBuffer ? currentBuffer.cursorLine : 1)
                                       ? theme.accent : theme.textMuted
                                height: settings.fontSize * 1.54
                                horizontalAlignment: Text.AlignRight
                                width: gutterWidth * 0.85
                            }
                        }
                    }
                }
            }

            // Separator
            Rectangle {
                anchors { left: gutterContainer.right; top: parent.top; bottom: parent.bottom }
                width: 1
                color: theme.border
                z: 0
            }

            // Text editor — anchored to right of gutter
            Flickable {
                id: editorFlick
                anchors {
                    left: gutterContainer.right
                    leftMargin: 1
                    top: parent.top
                    right: parent.right
                    bottom: parent.bottom
                }
                contentWidth: textEdit.paintedWidth + editorArea.width * 0.02
                contentHeight: textEdit.paintedHeight + editorArea.height * 0.03
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.AutoFlickIfNeeded
                z: 0

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
                    width: Math.max(editorFlick.width - editorArea.width * 0.02, implicitWidth)
                    padding: editorArea.width * 0.006
                    topPadding: editorArea.height * 0.008
                    font.family: settings.fontFamily
                    font.pixelSize: settings.fontSize
                    color: theme.text
                    selectionColor: theme.selection
                    selectedTextColor: theme.text
                    wrapMode: settings.wordWrap ? TextEdit.Wrap : TextEdit.NoWrap
                    selectByMouse: true
                    tabStopDistance: settings.tabWidth * fontMetrics.advanceWidth(' ')

                    FontMetrics { id: fontMetrics; font: textEdit.font }

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

                    Connections {
                        target: editorArea
                        function onCurrentBufferChanged() {
                            if (currentBuffer)
                                highlighter.language = currentBuffer.languageId()
                        }
                    }

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
                        var textBefore = text.substring(0, cursorPosition)
                        var lines = textBefore.split('\n')
                        currentBuffer.cursorLine = lines.length
                        currentBuffer.cursorColumn = lines[lines.length - 1].length + 1
                        if (lsp.running) completionTimer.restart()
                    }

                    Keys.onPressed: function(event) {
                        if (completionPopup.visible && (event.key === Qt.Key_Tab || event.key === Qt.Key_Return)) {
                            if (completionList.currentIndex >= 0 && completionList.currentIndex < lsp.completions.length) {
                                var lbl = lsp.completions[completionList.currentIndex]["label"]
                                if (lbl) textEdit.insert(textEdit.cursorPosition, lbl)
                            }
                            completionPopup.forceHide = true
                            event.accepted = true
                            return
                        }
                        if (completionPopup.visible && event.key === Qt.Key_Escape) {
                            completionPopup.forceHide = true
                            event.accepted = true
                            return
                        }
                        if (completionPopup.visible && event.key === Qt.Key_Down) {
                            completionList.currentIndex = Math.min(completionList.currentIndex + 1, completionList.count - 1)
                            event.accepted = true
                            return
                        }
                        if (completionPopup.visible && event.key === Qt.Key_Up) {
                            completionList.currentIndex = Math.max(completionList.currentIndex - 1, 0)
                            event.accepted = true
                            return
                        }
                        // Re-enable after typing next char
                        if (completionPopup.forceHide) completionPopup.forceHide = false
                    }

                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: editorContextMenu.popup()
                    }
                }
            }

            // Completion popup
            Rectangle {
                id: completionPopup
                property bool forceHide: false
                visible: !forceHide && lsp.completions.length > 0 && textEdit.activeFocus
                x: {
                    if (!currentBuffer) return gutterWidth + 1
                    var col = currentBuffer.cursorColumn - 1
                    var px = gutterWidth + 1 + (editorArea.width * 0.006) + col * charWidth - editorFlick.contentX
                    return Math.min(Math.max(gutterWidth + 1, px), editorBody.width - width - 4)
                }
                y: {
                    if (!currentBuffer) return 0
                    var line = currentBuffer.cursorLine
                    var py = (editorArea.height * 0.008) + line * (settings.fontSize * 1.54) - editorFlick.contentY
                    if (py + height > editorBody.height)
                        py = py - height - settings.fontSize * 1.54
                    return Math.max(0, Math.min(py, editorBody.height - height))
                }
                width: editorArea.width * 0.22
                height: Math.min(completionList.count * (editorArea.height * 0.033), editorArea.height * 0.25)
                color: theme.backgroundAlt
                border.color: theme.borderFocus
                border.width: 1
                radius: 4
                z: 100

                // Reset index when completions update
                onVisibleChanged: if (visible) completionList.currentIndex = 0

                ListView {
                    id: completionList
                    anchors { fill: parent; margins: 2 }
                    model: lsp.completions
                    clip: true
                    currentIndex: 0
                    delegate: Rectangle {
                        width: completionList.width
                        height: editorArea.height * 0.033
                        color: index === completionList.currentIndex ? theme.hover : "transparent"
                        RowLayout {
                            anchors { fill: parent; leftMargin: editorArea.width * 0.005; rightMargin: editorArea.width * 0.005 }
                            spacing: editorArea.width * 0.003
                            Text {
                                text: {
                                    var kind = modelData.kind || 0
                                    var kinds = {1:"txt",2:"fn",3:"fn",4:"ctr",5:"fld",6:"var",
                                                 7:"cls",8:"ifc",9:"mod",10:"prp",13:"enm",
                                                 14:"kw",15:"snp",21:"cst",22:"str"}
                                    return kinds[kind] || "·"
                                }
                                font.family: settings.fontFamily
                                font.pixelSize: editorArea.height * 0.012
                                color: theme.accent
                                Layout.preferredWidth: editorArea.width * 0.025
                            }
                            Text {
                                text: modelData.label || ""
                                font.family: settings.fontFamily
                                font.pixelSize: editorArea.height * 0.015
                                color: theme.text
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            Text {
                                text: modelData.detail || ""
                                font.family: settings.fontFamily
                                font.pixelSize: editorArea.height * 0.011
                                color: theme.textMuted
                                elide: Text.ElideRight
                                Layout.preferredWidth: editorArea.width * 0.055
                            }
                        }
                        TapHandler {
                            onTapped: {
                                var lbl = lsp.completions[index]["label"]
                                if (lbl) textEdit.insert(textEdit.cursorPosition, lbl)
                                completionPopup.forceHide = true
                            }
                        }
                        HoverHandler { onHoveredChanged: if (hovered) completionList.currentIndex = index }
                    }
                }
            }

            // Hover tooltip
            Rectangle {
                id: hoverTooltip
                visible: false
                x: editorArea.width * 0.1; y: editorArea.height * 0.1
                width: Math.min(hoverText.implicitWidth + editorArea.width * 0.015, editorArea.width * 0.5)
                height: hoverText.implicitHeight + editorArea.height * 0.02
                color: theme.backgroundAlt
                border.color: theme.borderFocus; border.width: 1; radius: 4
                z: 100

                Text {
                    id: hoverText
                    anchors { fill: parent; margins: editorArea.width * 0.007 }
                    font.family: settings.fontFamily
                    font.pixelSize: editorArea.height * 0.014
                    color: theme.text; wrapMode: Text.Wrap
                }

                Timer { id: hoverHideTimer; interval: 5000; onTriggered: hoverTooltip.visible = false }
            }

            Connections {
                target: lsp
                function onHoverResult(text) {
                    if (text) { hoverText.text = text; hoverTooltip.visible = true; hoverHideTimer.restart() }
                }
                function onDefinitionResult(uri, line, character) {
                    if (uri) openFile(uri.replace("file://", ""))
                }
            }

            Item {
                anchors.fill: parent
                visible: !currentBuffer
                Text {
                    anchors.centerIn: parent
                    text: "Ctrl+N new file · Ctrl+O open file"
                    font.family: settings.fontFamily
                    font.pixelSize: editorArea.height * 0.016
                    color: theme.textMuted
                }
            }
        }
    }

    Menu {
        id: editorContextMenu
        background: Rectangle {
            implicitWidth: editorArea.width * 0.16
            color: theme.backgroundAlt
            border.color: theme.borderFocus; border.width: 1; radius: 6
        }
        MenuItem {
            text: "Cut                  Ctrl+X"
            contentItem: Text { text: parent.text; font.family: settings.fontFamily; font.pixelSize: editorArea.height * 0.015; color: theme.text }
            background: Rectangle { color: parent.highlighted ? theme.hover : "transparent"; radius: 4 }
            onTriggered: textEdit.cut()
        }
        MenuItem {
            text: "Copy                 Ctrl+C"
            contentItem: Text { text: parent.text; font.family: settings.fontFamily; font.pixelSize: editorArea.height * 0.015; color: theme.text }
            background: Rectangle { color: parent.highlighted ? theme.hover : "transparent"; radius: 4 }
            onTriggered: textEdit.copy()
        }
        MenuItem {
            text: "Paste                Ctrl+V"
            contentItem: Text { text: parent.text; font.family: settings.fontFamily; font.pixelSize: editorArea.height * 0.015; color: theme.text }
            background: Rectangle { color: parent.highlighted ? theme.hover : "transparent"; radius: 4 }
            onTriggered: textEdit.paste()
        }
        MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: theme.border } }
        MenuItem {
            text: "Select All           Ctrl+A"
            contentItem: Text { text: parent.text; font.family: settings.fontFamily; font.pixelSize: editorArea.height * 0.015; color: theme.text }
            background: Rectangle { color: parent.highlighted ? theme.hover : "transparent"; radius: 4 }
            onTriggered: textEdit.selectAll()
        }
        MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: theme.border } }
        MenuItem {
            text: "Go to Definition     F12"
            enabled: lsp.running
            contentItem: Text { text: parent.text; font.family: settings.fontFamily; font.pixelSize: editorArea.height * 0.015; color: parent.enabled ? theme.text : theme.textMuted }
            background: Rectangle { color: parent.highlighted ? theme.hover : "transparent"; radius: 4 }
            onTriggered: { if (currentBuffer && lsp.running) lsp.requestDefinition("file://" + currentBuffer.filePath, currentBuffer.cursorLine - 1, currentBuffer.cursorColumn - 1) }
        }
        MenuItem {
            text: "Show Hover           Ctrl+H"
            enabled: lsp.running
            contentItem: Text { text: parent.text; font.family: settings.fontFamily; font.pixelSize: editorArea.height * 0.015; color: parent.enabled ? theme.text : theme.textMuted }
            background: Rectangle { color: parent.highlighted ? theme.hover : "transparent"; radius: 4 }
            onTriggered: { if (currentBuffer && lsp.running) lsp.requestHover("file://" + currentBuffer.filePath, currentBuffer.cursorLine - 1, currentBuffer.cursorColumn - 1) }
        }
        MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: theme.border } }
        MenuItem {
            text: "Save                 Ctrl+S"
            contentItem: Text { text: parent.text; font.family: settings.fontFamily; font.pixelSize: editorArea.height * 0.015; color: theme.text }
            background: Rectangle { color: parent.highlighted ? theme.hover : "transparent"; radius: 4 }
            onTriggered: { if (currentBuffer) currentBuffer.saveFile() }
        }
        MenuItem {
            text: "Close Tab            Ctrl+W"
            contentItem: Text { text: parent.text; font.family: settings.fontFamily; font.pixelSize: editorArea.height * 0.015; color: theme.text }
            background: Rectangle { color: parent.highlighted ? theme.hover : "transparent"; radius: 4 }
            onTriggered: closeCurrentTab()
        }
    }
}
