import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: explorerRoot
    signal fileSelected(string path)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header
        Rectangle {
            Layout.fillWidth: true
            height: 36
            color: theme.background
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 8
                spacing: 6
                Text {
                    text: "EXPLORER"
                    font.family: settings.fontFamily; font.pixelSize: 10
                    font.letterSpacing: 2; color: theme.textMuted
                    Layout.fillWidth: true
                }
                Rectangle {
                    width: 22; height: 22; radius: 4
                    color: refreshMa.containsMouse ? theme.border : "transparent"
                    Text { anchors.centerIn: parent; text: "⟳"; font.pixelSize: 12; color: theme.accent }
                    MouseArea {
                        id: refreshMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: fileSystemModel.refresh()
                    }
                }
            }
        }

        // Project name
        Rectangle {
            Layout.fillWidth: true
            height: projectManager.projectName ? 28 : 0
            color: "transparent"
            visible: projectManager.projectName !== ""
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 6
                Text {
                    text: projectManager.isGitRepo ? "⎇" : "◎"
                    font.pixelSize: 11; color: theme.accent
                }
                Text {
                    text: projectManager.projectName || ""
                    font.family: settings.fontFamily; font.pixelSize: 11
                    font.weight: Font.Bold; color: theme.textDim
                    elide: Text.ElideRight; Layout.fillWidth: true
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: theme.border }

        // File tree
        ListView {
            id: fileListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: fileSystemModel
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
                contentItem: Rectangle { implicitWidth: 4; radius: 2; color: theme.borderFocus }
                background: Item {}
            }

            delegate: Rectangle {
                id: fileDelegate
                width: fileListView.width
                height: 28
                color: delegateMa.containsMouse ? theme.hover : "transparent"
                Behavior on color { ColorAnimation { duration: 80 } }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8 + model.depth * 16
                    anchors.rightMargin: 8
                    spacing: 6

                    Text {
                        text: model.isDir ? (model.expanded ? "▾" : "▸") : " "
                        font.pixelSize: 10; color: theme.textMuted
                        Layout.preferredWidth: 12
                    }

                    Text {
                        text: model.isDir ? "📁" : fileIcon(model.name)
                        font.pixelSize: 11
                    }

                    Text {
                        text: model.name
                        font.family: settings.fontFamily; font.pixelSize: 12
                        color: model.isDir ? theme.textDim : theme.text
                        elide: Text.ElideRight; Layout.fillWidth: true
                    }
                }

                MouseArea {
                    id: delegateMa
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: function(mouse) {
                        if (mouse.button === Qt.RightButton) {
                            fileContextMenu.targetPath = model.fullPath
                            fileContextMenu.targetIsDir = model.isDir
                            fileContextMenu.targetName = model.name
                            fileContextMenu.targetIndex = model.index
                            fileContextMenu.popup()
                        } else {
                            if (model.isDir) {
                                fileSystemModel.toggleDir(index)
                            } else {
                                explorerRoot.fileSelected(model.fullPath)
                            }
                        }
                    }
                }
            }
        }

        // Empty state
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: fileSystemModel.rootPath === ""
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 8
                Text {
                    text: "No folder open"
                    font.family: settings.fontFamily; font.pixelSize: 12
                    color: theme.textMuted; Layout.alignment: Qt.AlignHCenter
                }
                Text {
                    text: "Ctrl+K to open"
                    font.family: settings.fontFamily; font.pixelSize: 11
                    color: theme.borderFocus; Layout.alignment: Qt.AlignHCenter
                }
            }
        }
    }

    // Context menu
    Menu {
        id: fileContextMenu
        property string targetPath: ""
        property bool targetIsDir: false
        property string targetName: ""
        property int targetIndex: -1

        background: Rectangle {
            implicitWidth: 200; color: theme.backgroundAlt
            border.color: theme.borderFocus; border.width: 1; radius: 6
        }

        MenuItem {
            text: fileContextMenu.targetIsDir ? "Open in Terminal" : "Open File"
            contentItem: Text {
                text: parent.text; font.family: settings.fontFamily
                font.pixelSize: 12; color: theme.text
            }
            background: Rectangle {
                color: parent.highlighted ? theme.border : "transparent"; radius: 4
            }
            onTriggered: {
                if (fileContextMenu.targetIsDir) {
                    dialogHelper.openTerminalAt(fileContextMenu.targetPath)
                } else {
                    explorerRoot.fileSelected(fileContextMenu.targetPath)
                }
            }
        }

        MenuItem {
            text: "Reveal in File Manager"
            contentItem: Text {
                text: parent.text; font.family: settings.fontFamily
                font.pixelSize: 12; color: theme.text
            }
            background: Rectangle {
                color: parent.highlighted ? theme.border : "transparent"; radius: 4
            }
            onTriggered: dialogHelper.revealInFiles(fileContextMenu.targetPath)
        }

        MenuSeparator {
            contentItem: Rectangle { implicitHeight: 1; color: theme.border }
        }

        MenuItem {
            text: "Copy Path"
            contentItem: Text {
                text: parent.text; font.family: settings.fontFamily
                font.pixelSize: 12; color: theme.text
            }
            background: Rectangle {
                color: parent.highlighted ? theme.border : "transparent"; radius: 4
            }
            onTriggered: dialogHelper.copyToClipboard(fileContextMenu.targetPath)
        }

        MenuItem {
            text: "Copy Relative Path"
            contentItem: Text {
                text: parent.text; font.family: settings.fontFamily
                font.pixelSize: 12; color: theme.text
            }
            background: Rectangle {
                color: parent.highlighted ? theme.border : "transparent"; radius: 4
            }
            onTriggered: {
                var rel = fileContextMenu.targetPath.replace(fileSystemModel.rootPath + "/", "")
                dialogHelper.copyToClipboard(rel)
            }
        }

        MenuSeparator {
            contentItem: Rectangle { implicitHeight: 1; color: theme.border }
        }

        MenuItem {
            visible: fileContextMenu.targetIsDir
            height: visible ? implicitHeight : 0
            text: "New File Here..."
            contentItem: Text {
                text: parent.text; font.family: settings.fontFamily
                font.pixelSize: 12; color: theme.text
            }
            background: Rectangle {
                color: parent.highlighted ? theme.border : "transparent"; radius: 4
            }
            onTriggered: {
                var newPath = dialogHelper.createNewFile(fileContextMenu.targetPath)
                if (newPath) {
                    fileSystemModel.refresh()
                    explorerRoot.fileSelected(newPath)
                }
            }
        }

        MenuItem {
            text: "Rename..."
            contentItem: Text {
                text: parent.text; font.family: settings.fontFamily
                font.pixelSize: 12; color: theme.text
            }
            background: Rectangle {
                color: parent.highlighted ? theme.border : "transparent"; radius: 4
            }
            onTriggered: {
                var newPath = dialogHelper.renameFile(fileContextMenu.targetPath)
                if (newPath) fileSystemModel.refresh()
            }
        }

        MenuItem {
            text: "Delete"
            contentItem: Text {
                text: parent.text; font.family: settings.fontFamily
                font.pixelSize: 12; color: theme.error
            }
            background: Rectangle {
                color: parent.highlighted ? theme.border : "transparent"; radius: 4
            }
            onTriggered: {
                if (dialogHelper.deleteFile(fileContextMenu.targetPath))
                    fileSystemModel.refresh()
            }
        }
    }

    function fileIcon(name) {
        var ext = name.split(".").pop().toLowerCase()
        var icons = {
            "cpp": "◆", "h": "◇", "hpp": "◇", "c": "◆",
            "py": "◆", "js": "◆", "ts": "◆", "rs": "◆",
            "qml": "◈", "html": "◉", "css": "◉",
            "json": "◎", "xml": "◎", "yaml": "◎", "yml": "◎",
            "md": "☰", "txt": "☰",
            "sh": "❯", "bash": "❯",
        }
        return icons[ext] || "○"
    }
}
