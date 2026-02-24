import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Window {
    id: root
    width: 1280
    height: 800
    visible: true
    title: "Slate"
    color: "#080C0F"

    Canvas {
        id: gridCanvas
        anchors.fill: parent
        opacity: 0.07
        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.strokeStyle = "#4ECDC4"
            ctx.lineWidth = 0.5
            var spacing = 40
            for (var x = 0; x <= width; x += spacing) {
                ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, height); ctx.stroke()
            }
            for (var y = 0; y <= height; y += spacing) {
                ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(width, y); ctx.stroke()
            }
        }
    }

    Canvas {
        width: 600; height: 600
        x: -150; y: -150
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
        x: root.width - 200; y: root.height - 200
        onPaint: {
            var ctx = getContext("2d")
            var grad = ctx.createRadialGradient(250, 250, 0, 250, 250, 250)
            grad.addColorStop(0, "#1A2A6B4A")
            grad.addColorStop(1, "transparent")
            ctx.fillStyle = grad
            ctx.fillRect(0, 0, width, height)
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            width: 220
            Layout.fillHeight: true
            color: "#0C1115"
            border.color: "#1A2832"
            border.width: 1

            ColumnLayout {
                anchors { fill: parent; margins: 24 }
                spacing: 0

                Item {
                    width: parent.width
                    height: 56
                    Layout.bottomMargin: 32

                    Canvas {
                        id: eyeGlyph
                        width: 32; height: 32
                        anchors.verticalCenter: parent.verticalCenter
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            ctx.beginPath()
                            ctx.moveTo(16, 2); ctx.lineTo(30, 16)
                            ctx.lineTo(16, 30); ctx.lineTo(2, 16)
                            ctx.closePath()
                            ctx.strokeStyle = "#4ECDC4"
                            ctx.lineWidth = 1.5
                            ctx.stroke()
                            ctx.beginPath()
                            ctx.arc(16, 16, 5, 0, Math.PI * 2)
                            ctx.fillStyle = "#4ECDC4"
                            ctx.fill()
                            ctx.beginPath()
                            ctx.moveTo(16, 21); ctx.lineTo(13, 28); ctx.lineTo(19, 28)
                            ctx.closePath()
                            ctx.fillStyle = "#4ECDC4"
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
                        anchors { left: eyeGlyph.right; leftMargin: 12; verticalCenter: parent.verticalCenter }
                        font.family: "Courier New"
                        font.pixelSize: 18
                        font.letterSpacing: 6
                        font.weight: Font.Bold
                        color: "#E8F4F3"
                    }
                }

                Repeater {
                    model: [
                        { icon: "◈", label: "New File",    shortcut: "Ctrl+N" },
                        { icon: "◉", label: "Open File",   shortcut: "Ctrl+O" },
                        { icon: "◎", label: "Open Folder", shortcut: "Ctrl+K" },
                        { icon: "◇", label: "Recent",      shortcut: "Ctrl+R" },
                    ]
                    delegate: Rectangle {
                        width: parent.width
                        height: 44
                        Layout.bottomMargin: 4
                        radius: 6
                        color: navHover.containsMouse ? "#162028" : "transparent"
                        Behavior on color { ColorAnimation { duration: 120 } }
                        RowLayout {
                            anchors { fill: parent; leftMargin: 12; rightMargin: 12 }
                            spacing: 10
                            Text { text: modelData.icon; font.pixelSize: 14; color: "#4ECDC4" }
                            Text {
                                text: modelData.label
                                font.family: "Courier New"
                                font.pixelSize: 13
                                color: "#A8C4C2"
                                Layout.fillWidth: true
                            }
                            Text {
                                text: modelData.shortcut
                                font.family: "Courier New"
                                font.pixelSize: 11
                                color: "#3D5C5A"
                            }
                        }
                        HoverHandler { id: navHover }
                        TapHandler { onTapped: console.log("nav:", modelData.label) }
                    }
                }

                Item { Layout.fillHeight: true }

                Repeater {
                    model: [
                        { icon: "⌥", label: "Settings" },
                        { icon: "?", label: "Help" },
                    ]
                    delegate: Rectangle {
                        width: parent.width
                        height: 40
                        radius: 6
                        color: btmHover.containsMouse ? "#162028" : "transparent"
                        Behavior on color { ColorAnimation { duration: 120 } }
                        RowLayout {
                            anchors { fill: parent; leftMargin: 12; rightMargin: 12 }
                            spacing: 10
                            Text { text: modelData.icon; font.pixelSize: 13; color: "#4ECDC4" }
                            Text {
                                text: modelData.label
                                font.family: "Courier New"
                                font.pixelSize: 13
                                color: "#A8C4C2"
                            }
                        }
                        HoverHandler { id: btmHover }
                    }
                }

                Item { height: 8 }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors { fill: parent; margins: 48 }
                spacing: 0

                ColumnLayout {
                    spacing: 8
                    Layout.bottomMargin: 48

                    Text {
                        text: "// welcome back"
                        font.family: "Courier New"
                        font.pixelSize: 12
                        color: "#2A6B4A"
                        font.letterSpacing: 2
                        NumberAnimation on opacity {
                            running: true; from: 0; to: 1
                            duration: 600; easing.type: Easing.OutCubic
                        }
                    }

                    Text {
                        text: "Start here."
                        font.family: "Courier New"
                        font.pixelSize: 42
                        font.weight: Font.Bold
                        color: "#E8F4F3"
                        font.letterSpacing: -1
                        NumberAnimation on opacity {
                            running: true; from: 0; to: 1

                        }
                        NumberAnimation on y {
                            running: true; from: 10; to: 0

                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 32

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Text {
                            text: "RECENT FILES"
                            font.family: "Courier New"
                            font.pixelSize: 11
                            font.letterSpacing: 3
                            color: "#3D6B68"
                            Layout.bottomMargin: 4
                        }

                        Repeater {
                            model: [
                                { name: "main.cpp",       path: "~/slate/src/",      lang: "C++",    time: "2m ago" },
                                { name: "editor.qml",     path: "~/slate/qml/",      lang: "QML",    time: "1h ago" },
                                { name: "lsp_client.cpp", path: "~/slate/src/lsp/",  lang: "C++",    time: "3h ago" },
                                { name: "CMakeLists.txt", path: "~/slate/",          lang: "CMake",  time: "yesterday" },
                                { name: "buffer.h",       path: "~/slate/include/",  lang: "Header", time: "2d ago" },
                            ]
                            delegate: Rectangle {
                                width: parent.width
                                height: 56
                                radius: 8
                                color: fileHover.containsMouse ? "#0F1D24" : "#0A1419"
                                border.color: fileHover.containsMouse ? "#2A4A48" : "#141E25"
                                border.width: 1
                                Behavior on color { ColorAnimation { duration: 120 } }
                                Behavior on border.color { ColorAnimation { duration: 120 } }

                                Rectangle {
                                    width: 3; height: 32
                                    anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                                    radius: 2
                                    color: fileHover.containsMouse ? "#4ECDC4" : "#1E3A38"
                                    Behavior on color { ColorAnimation { duration: 120 } }
                                }

                                RowLayout {
                                    anchors { fill: parent; leftMargin: 20; rightMargin: 16 }
                                    spacing: 12
                                    ColumnLayout {
                                        spacing: 3
                                        Layout.fillWidth: true
                                        Text {
                                            text: modelData.name
                                            font.family: "Courier New"
                                            font.pixelSize: 13
                                            font.weight: Font.Medium
                                            color: "#D4ECEB"
                                        }
                                        Text {
                                            text: modelData.path
                                            font.family: "Courier New"
                                            font.pixelSize: 11
                                            color: "#3D6B68"
                                        }
                                    }
                                    Rectangle {
                                        height: 20
                                        width: langLabel.width + 16
                                        radius: 4
                                        color: "#0C2422"
                                        border.color: "#1A4A47"
                                        border.width: 1
                                        Text {
                                            id: langLabel
                                            anchors.centerIn: parent
                                            text: modelData.lang
                                            font.family: "Courier New"
                                            font.pixelSize: 10
                                            color: "#4ECDC4"
                                            font.letterSpacing: 1
                                        }
                                    }
                                    Text {
                                        text: modelData.time
                                        font.family: "Courier New"
                                        font.pixelSize: 11
                                        color: "#2A4A48"
                                    }
                                }

                                HoverHandler { id: fileHover }
                                TapHandler { onTapped: console.log("open:", modelData.name) }

                                NumberAnimation on opacity {
                                    running: true; from: 0; to: 1

                                    easing.type: Easing.OutCubic
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        width: 280
                        Layout.preferredWidth: 280
                        spacing: 20

                        ColumnLayout {
                            width: parent.width
                            spacing: 12

                            Text {
                                text: "QUICK ACTIONS"
                                font.family: "Courier New"
                                font.pixelSize: 11
                                font.letterSpacing: 3
                                color: "#3D6B68"
                                Layout.bottomMargin: 4
                            }

                            Repeater {
                                model: [
                                    { label: "Clone Repository", icon: "⎇" },
                                    { label: "Command Palette",  icon: "❯" },
                                    { label: "Configure LSP",    icon: "◈" },
                                ]
                                delegate: Rectangle {
                                    width: 280
                                    height: 44
                                    radius: 8
                                    color: actHover.containsMouse ? "#0F1D24" : "#0A1419"
                                    border.color: actHover.containsMouse ? "#2A4A48" : "#141E25"
                                    border.width: 1
                                    Behavior on color { ColorAnimation { duration: 120 } }
                                    RowLayout {
                                        anchors { fill: parent; leftMargin: 16; rightMargin: 16 }
                                        spacing: 12
                                        Text { text: modelData.icon; font.pixelSize: 14; color: "#4ECDC4" }
                                        Text {
                                            text: modelData.label
                                            font.family: "Courier New"
                                            font.pixelSize: 13
                                            color: "#A8C4C2"
                                        }
                                    }
                                    HoverHandler { id: actHover }
                                }
                            }
                        }

                        Rectangle {
                            width: 280
                            height: tipCol.implicitHeight + 32
                            radius: 8
                            color: "#07100D"
                            border.color: "#1A3830"
                            border.width: 1
                            ColumnLayout {
                                id: tipCol
                                anchors { fill: parent; margins: 16 }
                                spacing: 8
                                Text {
                                    text: "// tip"
                                    font.family: "Courier New"
                                    font.pixelSize: 11
                                    font.letterSpacing: 2
                                    color: "#2A6B4A"
                                }
                                Text {
                                    text: "Press Ctrl+P to open the command palette at any time."
                                    font.family: "Courier New"
                                    font.pixelSize: 12
                                    color: "#5A8A82"
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                    lineHeight: 1.5
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }

                        Text {
                            text: "slate v0.1.0 — sheikah build"
                            font.family: "Courier New"
                            font.pixelSize: 10
                            color: "#1E3A38"
                            font.letterSpacing: 1
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }
    }

    Canvas {
        anchors.fill: parent
        opacity: 0.025
        onPaint: {
            var ctx = getContext("2d")
            for (var y = 0; y < height; y += 3) {
                ctx.fillStyle = "#000000"
                ctx.fillRect(0, y, width, 1)
            }
        }
    }
}
