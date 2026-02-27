import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: welcomeRoot

    signal openFile()
    signal openFolder()
    signal newFile()
    signal projectOpened(string path)

    // Glow top-left
    Canvas {
        width: 600; height: 600; x: -150; y: -150
        onPaint: {
            var ctx = getContext("2d")
            var grad = ctx.createRadialGradient(300, 300, 0, 300, 300, 300)
            grad.addColorStop(0, "#1A4ECDC4")
            grad.addColorStop(1, "transparent")
            ctx.fillStyle = grad
            ctx.fillRect(0, 0, width, height)
        }
    }

    Canvas {
        width: 500; height: 500
        x: welcomeRoot.width - 200; y: welcomeRoot.height - 200
        onPaint: {
            var ctx = getContext("2d")
            var grad = ctx.createRadialGradient(250, 250, 0, 250, 250, 250)
            grad.addColorStop(0, "#1A2A6B4A")
            grad.addColorStop(1, "transparent")
            ctx.fillStyle = grad
            ctx.fillRect(0, 0, width, height)
        }
    }

    ColumnLayout {
        anchors {
            fill: parent
            leftMargin: 64
            rightMargin: 64
            topMargin: 48
            bottomMargin: 48
        }
        spacing: 0

        // Logo
        RowLayout {
            spacing: 12
            Layout.bottomMargin: 24
            Canvas {
                id: eyeGlyph
                width: 32; height: 32
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    ctx.beginPath()
                    ctx.moveTo(16, 2); ctx.lineTo(30, 16)
                    ctx.lineTo(16, 30); ctx.lineTo(2, 16)
                    ctx.closePath()
                    ctx.strokeStyle = theme.accent
                    ctx.lineWidth = 1.5
                    ctx.stroke()
                    ctx.beginPath()
                    ctx.arc(16, 16, 5, 0, Math.PI * 2)
                    ctx.fillStyle = theme.accent
                    ctx.fill()
                }
                SequentialAnimation on opacity {
                    running: true; loops: Animation.Infinite
                    NumberAnimation { to: 0.4; duration: 2000; easing.type: Easing.InOutSine }
                    NumberAnimation { to: 1.0; duration: 2000; easing.type: Easing.InOutSine }
                }
            }
            Text {
                text: "SLATE"
                font.family: settings.fontFamily; font.pixelSize: 18
                font.letterSpacing: 6; font.weight: Font.Bold
                color: theme.text
            }
        }

        Text {
            text: "// welcome back"
            font.family: settings.fontFamily; font.pixelSize: 12
            color: theme.comment; font.letterSpacing: 2
            Layout.bottomMargin: 4
        }

        Text {
            text: "Start here."
            font.family: settings.fontFamily; font.pixelSize: 42
            font.weight: Font.Bold; color: theme.text
            font.letterSpacing: -1
            Layout.bottomMargin: 40
        }

        // Two-column content
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 40

            // LEFT: Actions
            ColumnLayout {
                Layout.preferredWidth: 340
                Layout.alignment: Qt.AlignTop
                spacing: 8

                Text {
                    text: "ACTIONS"
                    font.family: settings.fontFamily; font.pixelSize: 11
                    font.letterSpacing: 3; color: theme.textMuted
                    Layout.bottomMargin: 8
                }

                // New File
                Rectangle {
                    id: newBtn
                    Layout.fillWidth: true
                    height: 48; radius: 8
                    color: newMa.containsMouse ? theme.surfaceAlt : theme.surface
                    border.color: newMa.containsMouse ? theme.borderFocus : theme.border
                    border.width: 1
                    Behavior on color { ColorAnimation { duration: 120 } }
                    Behavior on border.color { ColorAnimation { duration: 120 } }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        spacing: 12
                        Text { text: "◈"; font.pixelSize: 14; color: theme.accent }
                        Text {
                            text: "New File"; font.family: settings.fontFamily
                            font.pixelSize: 13; color: theme.textDim
                            Layout.fillWidth: true
                        }
                        Text {
                            text: "Ctrl+N"; font.family: settings.fontFamily
                            font.pixelSize: 11; color: theme.textMuted
                        }
                    }

                    MouseArea {
                        id: newMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: welcomeRoot.newFile()
                    }
                }

                // Open File
                Rectangle {
                    Layout.fillWidth: true
                    height: 48; radius: 8
                    color: fileMa.containsMouse ? theme.surfaceAlt : theme.surface
                    border.color: fileMa.containsMouse ? theme.borderFocus : theme.border
                    border.width: 1
                    Behavior on color { ColorAnimation { duration: 120 } }
                    Behavior on border.color { ColorAnimation { duration: 120 } }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        spacing: 12
                        Text { text: "◉"; font.pixelSize: 14; color: theme.accent }
                        Text {
                            text: "Open File"; font.family: settings.fontFamily
                            font.pixelSize: 13; color: theme.textDim
                            Layout.fillWidth: true
                        }
                        Text {
                            text: "Ctrl+O"; font.family: settings.fontFamily
                            font.pixelSize: 11; color: theme.textMuted
                        }
                    }

                    MouseArea {
                        id: fileMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: welcomeRoot.openFile()
                    }
                }

                // Open Folder
                Rectangle {
                    Layout.fillWidth: true
                    height: 48; radius: 8
                    color: folderMa.containsMouse ? theme.surfaceAlt : theme.surface
                    border.color: folderMa.containsMouse ? theme.borderFocus : theme.border
                    border.width: 1
                    Behavior on color { ColorAnimation { duration: 120 } }
                    Behavior on border.color { ColorAnimation { duration: 120 } }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        spacing: 12
                        Text { text: "◎"; font.pixelSize: 14; color: theme.accent }
                        Text {
                            text: "Open Folder"; font.family: settings.fontFamily
                            font.pixelSize: 13; color: theme.textDim
                            Layout.fillWidth: true
                        }
                        Text {
                            text: "Ctrl+K"; font.family: settings.fontFamily
                            font.pixelSize: 11; color: theme.textMuted
                        }
                    }

                    MouseArea {
                        id: folderMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: welcomeRoot.openFolder()
                    }
                }
            }

            // RIGHT: Recent Projects + Tip
            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                spacing: 8

                Text {
                    text: "RECENT PROJECTS"
                    font.family: settings.fontFamily; font.pixelSize: 11
                    font.letterSpacing: 3; color: theme.textMuted
                    Layout.bottomMargin: 8
                }

                Repeater {
                    model: projectManager.recentProjects
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        height: 44; radius: 6
                        color: rpMa.containsMouse ? theme.surfaceAlt : theme.surface
                        border.color: rpMa.containsMouse ? theme.borderFocus : theme.border
                        border.width: 1
                        Behavior on color { ColorAnimation { duration: 120 } }
                        Behavior on border.color { ColorAnimation { duration: 120 } }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 8
                            Text { text: "◎"; font.pixelSize: 12; color: theme.accent }
                            ColumnLayout {
                                spacing: 2
                                Layout.fillWidth: true
                                Text {
                                    text: {
                                        var parts = modelData.split("/")
                                        return parts[parts.length - 1] || modelData
                                    }
                                    font.family: settings.fontFamily; font.pixelSize: 12
                                    font.weight: Font.Medium; color: theme.textDim
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: modelData
                                    font.family: settings.fontFamily; font.pixelSize: 10
                                    color: theme.borderFocus; elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                }
                            }
                        }

                        MouseArea {
                            id: rpMa
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: welcomeRoot.projectOpened(modelData)
                        }
                    }
                }

                // Tip box
                Rectangle {
                    Layout.fillWidth: true
                    height: tipContent.implicitHeight + 32
                    radius: 8; color: theme.background
                    border.color: theme.border; border.width: 1
                    Layout.topMargin: 16

                    ColumnLayout {
                        id: tipContent
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 8
                        Text {
                            text: "// tip"; font.family: settings.fontFamily
                            font.pixelSize: 11; font.letterSpacing: 2; color: theme.comment
                        }
                        Text {
                            text: "Press Ctrl+B to toggle the file explorer.\nCtrl+P opens the command palette."
                            font.family: settings.fontFamily; font.pixelSize: 12
                            color: theme.textDim; wrapMode: Text.WordWrap
                            Layout.fillWidth: true; lineHeight: 1.5
                        }
                    }
                }
            }
        }

        // Footer
        Item { Layout.fillHeight: true }
        Text {
            text: "slate v0.1.0 — sheikah build"
            font.family: settings.fontFamily; font.pixelSize: 10
            color: theme.textMuted; font.letterSpacing: 1
        }
    }
}
