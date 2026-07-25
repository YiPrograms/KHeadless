pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kheadless

Item {
    id: root

    DashboardController {
        id: controller
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.preferredWidth: 230
            Layout.fillHeight: true
            color: palette.alternateBase

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 8

                Label {
                    text: qsTr("KHeadless")
                    font.pixelSize: 24
                    font.bold: true
                }
                Label {
                    text: controller.available ? qsTr("Plasma remote workspace") : qsTr("Daemon unavailable")
                    color: controller.available ? palette.mid : "#d84a4a"
                    Layout.bottomMargin: 18
                }

                Repeater {
                    model: [
                        { label: qsTr("Overview"), icon: "⌂" },
                        { label: qsTr("Displays"), icon: "▣" },
                        { label: qsTr("Access"), icon: "●" },
                        { label: qsTr("Diagnostics"), icon: "≡" }
                    ]
                    delegate: Button {
                        required property var modelData
                        required property int index
                        Layout.fillWidth: true
                        checkable: true
                        checked: pages.currentIndex === index
                        text: modelData.icon + "   " + modelData.label
                        onClicked: pages.currentIndex = index
                    }
                }

                Item { Layout.fillHeight: true }

                Label {
                    Layout.fillWidth: true
                    visible: controller.lastError.length > 0
                    text: controller.lastError
                    color: "#d84a4a"
                    wrapMode: Text.Wrap
                    font.pixelSize: 12
                }
            }
        }

        StackLayout {
            id: pages
            Layout.fillWidth: true
            Layout.fillHeight: true

            OverviewPage { controller: controller }
            DisplaysPage { controller: controller }
            AccessPage { controller: controller }
            DiagnosticsPage { controller: controller }
        }
    }

    Rectangle {
        visible: controller.confirmationPending
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        width: confirmRow.implicitWidth + 36
        height: 58
        radius: 10
        color: palette.window
        border.color: palette.highlight
        z: 100

        RowLayout {
            id: confirmRow
            anchors.centerIn: parent
            Label { text: qsTr("Keep this layout? Reverting in %1 s").arg(controller.confirmationSeconds) }
            Button { text: qsTr("Revert"); onClicked: controller.revertLayout() }
            Button { text: qsTr("Keep"); highlighted: true; onClicked: controller.confirmLayout() }
        }
    }
}
