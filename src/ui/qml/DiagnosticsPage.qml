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
        anchors.margins: 28
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            Label { text: qsTr("Diagnostics"); font.pixelSize: 28; font.bold: true; Layout.fillWidth: true }
            Button { text: qsTr("Refresh"); onClicked: root.controller.refresh() }
        }

        Repeater {
            model: Object.keys(root.controller.diagnostics).sort()
            delegate: Rectangle {
                id: diagnosticRow
                required property string modelData
                required property int index
                Layout.fillWidth: true
                Layout.preferredHeight: 54
                color: diagnosticRow.index % 2 ? palette.alternateBase : palette.base
                radius: 6
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    Label { text: diagnosticRow.modelData; Layout.preferredWidth: 220; font.bold: true }
                    Label {
                        Layout.fillWidth: true
                        text: String(root.controller.diagnostics[diagnosticRow.modelData])
                        elide: Text.ElideMiddle
                        color: palette.mid
                    }
                }
            }
        }
    }
}
