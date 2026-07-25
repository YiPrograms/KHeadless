pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    id: root
    required property var controller
    contentWidth: availableWidth

    ColumnLayout {
        width: parent.width
        spacing: 20
        anchors.margins: 28

        Label {
            text: qsTr("Remote workspace")
            font.pixelSize: 28
            font.bold: true
        }
        Label {
            Layout.fillWidth: true
            text: qsTr("A host-native Plasma session delivered through RDP. Applications retain normal access to your files, devices, and desktop services.")
            wrapMode: Text.Wrap
            color: palette.mid
        }

        GridLayout {
            columns: 2
            columnSpacing: 16
            rowSpacing: 16
            Layout.fillWidth: true

            Repeater {
                model: [
                    { title: qsTr("Server"), value: root.controller.running ? qsTr("Running") : qsTr("Stopped"), detail: root.controller.running ? qsTr("Accepting encrypted RDP connections") : qsTr("Start after adding an RDP user") },
                    { title: qsTr("Layout mode"), value: root.controller.mode, detail: qsTr("%1 configured displays").arg(root.controller.monitors.length) },
                    { title: qsTr("Connections"), value: String(root.controller.connections.length), detail: root.controller.controller.length ? qsTr("Controller %1").arg(root.controller.controller) : qsTr("No active controller") },
                    { title: qsTr("Backend"), value: root.controller.diagnostics.rdpBackend || qsTr("Unknown"), detail: root.controller.diagnostics.outputBackend || "" }
                ]
                delegate: Rectangle {
                    id: card
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.preferredHeight: 125
                    radius: 12
                    color: palette.alternateBase
                    border.color: palette.midlight

                    Column {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 5
                        Label { text: card.modelData.title; color: palette.mid }
                        Label { text: card.modelData.value; font.pixelSize: 23; font.bold: true }
                        Label { text: card.modelData.detail; color: palette.mid; width: parent.width; elide: Text.ElideRight }
                    }
                }
            }
        }

        RowLayout {
            Button {
                text: root.controller.running ? qsTr("Stop server") : qsTr("Start server")
                highlighted: !root.controller.running
                onClicked: root.controller.running ? root.controller.stopServer() : root.controller.startServer()
            }
            Button { text: qsTr("Refresh"); onClicked: root.controller.refresh() }
        }
    }
}
