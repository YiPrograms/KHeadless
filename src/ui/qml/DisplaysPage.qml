pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    required property var controller
    property int selectedIndex: controller.monitors.length ? 0 : -1
    readonly property var displayBounds: {
        let minimumX = 0
        let minimumY = 0
        let maximumX = 1920
        let maximumY = 1080
        let first = true
        for (const monitor of controller.monitors) {
            if (!monitor.enabled)
                continue
            const rotated = monitor.rotation === 90 || monitor.rotation === 270
            const width = rotated ? monitor.height : monitor.width
            const height = rotated ? monitor.width : monitor.height
            minimumX = first ? monitor.x : Math.min(minimumX, monitor.x)
            minimumY = first ? monitor.y : Math.min(minimumY, monitor.y)
            maximumX = first ? monitor.x + width : Math.max(maximumX, monitor.x + width)
            maximumY = first ? monitor.y + height : Math.max(maximumY, monitor.y + height)
            first = false
        }
        return { left: minimumX, top: minimumY,
                 width: Math.max(1, maximumX - minimumX),
                 height: Math.max(1, maximumY - minimumY) }
    }
    readonly property real canvasScale: Math.max(0.01, Math.min(0.25,
        (canvas.width - 64) / displayBounds.width,
        (canvas.height - 64) / displayBounds.height))

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 14

        RowLayout {
            Layout.fillWidth: true
            Label { text: qsTr("Displays"); font.pixelSize: 28; font.bold: true }
            Item { Layout.fillWidth: true }
            ComboBox {
                model: [qsTr("Follow RDP client"), qsTr("Manual"), qsTr("Saved profile")]
                currentIndex: root.controller.mode === "manual" ? 1 : root.controller.mode === "profile" ? 2 : 0
                onActivated: root.controller.setMode(["follow-client", "manual", "profile"][currentIndex])
            }
            Button { text: qsTr("Add display"); enabled: root.controller.monitors.length < 16; onClicked: root.controller.addMonitor() }
        }

        Label {
            text: qsTr("Drag displays to arrange them. Client-driven changes are applied automatically in Follow RDP client mode.")
            color: palette.mid
        }

        Rectangle {
            id: canvas
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 270
            color: Qt.rgba(palette.alternateBase.r, palette.alternateBase.g, palette.alternateBase.b, 0.55)
            radius: 12
            border.color: palette.midlight
            clip: true
            property real originX: (width - root.displayBounds.width * root.canvasScale) / 2
                                   - root.displayBounds.left * root.canvasScale
            property real originY: (height - root.displayBounds.height * root.canvasScale) / 2
                                   - root.displayBounds.top * root.canvasScale

            Repeater {
                model: root.controller.monitors
                delegate: Rectangle {
                    id: display
                    required property var modelData
                    required property int index
                    visible: display.modelData.enabled
                    x: canvas.originX + display.modelData.x * root.canvasScale
                    y: canvas.originY + display.modelData.y * root.canvasScale
                    readonly property bool rotated: display.modelData.rotation === 90 || display.modelData.rotation === 270
                    width: Math.max(60, (rotated ? display.modelData.height : display.modelData.width) * root.canvasScale)
                    height: Math.max(42, (rotated ? display.modelData.width : display.modelData.height) * root.canvasScale)
                    radius: 7
                    color: root.selectedIndex === index ? palette.highlight : palette.button
                    border.width: display.modelData.primary ? 3 : 1
                    border.color: display.modelData.primary ? palette.highlightedText : palette.mid

                    Column {
                        anchors.centerIn: parent
                        Label {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: display.modelData.name
                            color: root.selectedIndex === display.index ? palette.highlightedText : palette.buttonText
                            font.bold: true
                        }
                        Label {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: display.modelData.width + " × " + display.modelData.height + (display.modelData.primary ? qsTr("  Primary") : "")
                            color: root.selectedIndex === display.index ? palette.highlightedText : palette.mid
                            font.pixelSize: 11
                        }
                    }

                    TapHandler { onTapped: root.selectedIndex = display.index }
                    DragHandler {
                        id: drag
                        onActiveChanged: {
                            if (!active) {
                                root.controller.updateMonitorPosition(display.index,
                                    Math.round((display.x - canvas.originX) / root.canvasScale / 10) * 10,
                                    Math.round((display.y - canvas.originY) / root.canvasScale / 10) * 10)
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            id: editor
            Layout.fillWidth: true
            Layout.preferredHeight: 126
            visible: root.selectedIndex >= 0 && root.selectedIndex < root.controller.monitors.length
            color: palette.alternateBase
            radius: 10

            property var selected: visible ? root.controller.monitors[root.selectedIndex] : ({})

            RowLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 12

                ColumnLayout {
                    Label { text: qsTr("Resolution"); color: palette.mid }
                    RowLayout {
                        SpinBox {
                            from: 200; to: 8192; editable: true
                            value: editor.selected.width || 1920
                            onValueModified: root.controller.updateMonitor(root.selectedIndex, "width", value)
                        }
                        Label { text: "×" }
                        SpinBox {
                            from: 200; to: 8192; editable: true
                            value: editor.selected.height || 1080
                            onValueModified: root.controller.updateMonitor(root.selectedIndex, "height", value)
                        }
                    }
                }
                ColumnLayout {
                    Label { text: qsTr("Scale"); color: palette.mid }
                    ComboBox {
                        model: ["0.5", "0.75", "1.0", "1.25", "1.5", "2.0", "2.5", "3.0", "4.0"]
                        currentIndex: {
                            for (let index = 0; index < model.length; ++index) {
                                if (Number(model[index]) === Number(editor.selected.scale))
                                    return index
                            }
                            return 2
                        }
                        onActivated: root.controller.updateMonitor(root.selectedIndex, "scale", Number(currentText))
                    }
                }
                ColumnLayout {
                    Label { text: qsTr("Rotation"); color: palette.mid }
                    ComboBox {
                        model: ["0°", "90°", "180°", "270°"]
                        currentIndex: [0, 90, 180, 270].indexOf(editor.selected.rotation)
                        onActivated: root.controller.updateMonitor(root.selectedIndex, "rotation", [0, 90, 180, 270][currentIndex])
                    }
                }
                Item { Layout.fillWidth: true }
                ColumnLayout {
                    Button { text: qsTr("Make primary"); enabled: !editor.selected.primary; onClicked: root.controller.makePrimary(root.selectedIndex) }
                    Button { text: qsTr("Remove"); enabled: root.controller.monitors.length > 1; onClicked: { root.controller.removeMonitor(root.selectedIndex); root.selectedIndex = 0 } }
                }
            }
        }

        RowLayout {
            Item { Layout.fillWidth: true }
            Button { text: qsTr("Reload"); onClicked: root.controller.reloadLayout() }
            Button { text: qsTr("Apply"); highlighted: true; onClicked: root.controller.applyLayout(true) }
        }
    }
}
