import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: toastRoot
    anchors.fill: parent
    z: 9999

    // Toast queue
    ListModel { id: toastModel }

    function show(message, type) {
        // type: "info", "success", "error", "warning"
        toastModel.append({ message: message, type: type || "info" })
        // Auto-remove after 4s
        Qt.callLater(function() {
            removeTimer.restart()
        })
    }

    Timer {
        id: removeTimer
        interval: 4000
        repeat: false
        onTriggered: {
            if (toastModel.count > 0)
                toastModel.remove(0)
            if (toastModel.count > 0)
                removeTimer.restart()
        }
    }

    Column {
        anchors {
            bottom: parent.bottom
            right: parent.right
            bottomMargin: 40
            rightMargin: 16
        }
        spacing: 6
        width: 340

        Repeater {
            model: toastModel
            delegate: Rectangle {
                width: 340
                height: toastContent.implicitHeight + 16
                radius: 6
                color: theme.backgroundAlt
                border.color: {
                    switch(model.type) {
                        case "success": return theme.success
                        case "error": return theme.error
                        case "warning": return theme.warning
                        default: return theme.border
                    }
                }
                border.width: 1
                opacity: 0

                Component.onCompleted: opacity = 1
                Behavior on opacity { NumberAnimation { duration: 200 } }

                RowLayout {
                    id: toastContent
                    anchors {
                        fill: parent
                        leftMargin: 10; rightMargin: 10
                        topMargin: 8; bottomMargin: 8
                    }
                    spacing: 8

                    Text {
                        text: {
                            switch(model.type) {
                                case "success": return "✓"
                                case "error": return "✕"
                                case "warning": return "⚠"
                                default: return "ℹ"
                            }
                        }
                        font.pixelSize: 13
                        color: {
                            switch(model.type) {
                                case "success": return theme.success
                                case "error": return theme.error
                                case "warning": return theme.warning
                                default: return theme.accent
                            }
                        }
                    }

                    Text {
                        text: model.message
                        font.family: settings.fontFamily
                        font.pixelSize: 12
                        color: theme.text
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Rectangle {
                        width: 18; height: 18; radius: 3
                        color: dismissMa.containsMouse ? theme.hover : "transparent"
                        Text {
                            anchors.centerIn: parent; text: "×"
                            font.pixelSize: 12; color: theme.textMuted
                        }
                        MouseArea {
                            id: dismissMa; anchors.fill: parent
                            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            onClicked: toastModel.remove(index)
                        }
                    }
                }
            }
        }
    }
}
