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
        spacing: 18

        Label { text: qsTr("Access and profiles"); font.pixelSize: 28; font.bold: true }

        GroupBox {
            title: qsTr("RDP server")
            Layout.fillWidth: true
            GridLayout {
                anchors.fill: parent
                columns: 4

                Label { text: qsTr("Listen address") }
                TextField {
                    Layout.fillWidth: true
                    text: root.controller.serverConfiguration.address || "0.0.0.0"
                    onEditingFinished: root.controller.updateServerSetting("address", text)
                }
                Label { text: qsTr("Port") }
                SpinBox {
                    from: 1
                    to: 65535
                    editable: true
                    value: root.controller.serverConfiguration.port || 3389
                    onValueModified: root.controller.updateServerSetting("port", value)
                }

                Label { text: qsTr("Video quality") }
                Slider {
                    Layout.fillWidth: true
                    from: 0
                    to: 100
                    stepSize: 1
                    value: root.controller.serverConfiguration.quality || 80
                    onMoved: root.controller.updateServerSetting("quality", Math.round(value))
                }
                CheckBox {
                    text: qsTr("Playback audio")
                    enabled: root.controller.diagnostics.audioPlaybackTransport ?? false
                    checked: root.controller.serverConfiguration.audio ?? true
                    onToggled: root.controller.updateServerSetting("audio", checked)
                    ToolTip.visible: hovered && !enabled
                    ToolTip.text: qsTr("Requires the patched embedded KRdp backend")
                }
                CheckBox {
                    text: qsTr("Clipboard")
                    checked: root.controller.serverConfiguration.clipboard ?? true
                    onToggled: root.controller.updateServerSetting("clipboard", checked)
                }

                Item { Layout.columnSpan: 2; Layout.fillWidth: true }
                Button { text: qsTr("Reload"); onClicked: root.controller.reloadServerConfiguration() }
                Button { text: qsTr("Apply server settings"); onClicked: root.controller.applyServerSettings() }
            }
        }

        GroupBox {
            title: qsTr("RDP users")
            Layout.fillWidth: true
            ColumnLayout {
                anchors.fill: parent
                Repeater {
                    model: root.controller.users
                    delegate: RowLayout {
                        id: userRow
                        required property string modelData
                        Layout.fillWidth: true
                        Label { text: userRow.modelData; Layout.fillWidth: true }
                        Button { text: qsTr("Remove"); onClicked: root.controller.deleteUser(userRow.modelData) }
                    }
                }
                RowLayout {
                    TextField { id: username; placeholderText: qsTr("Username"); Layout.fillWidth: true }
                    TextField { id: password; placeholderText: qsTr("Password (8+ characters)"); echoMode: TextInput.Password; Layout.fillWidth: true }
                    Button {
                        text: qsTr("Add or update")
                        enabled: username.text.length > 0 && password.text.length >= 8
                        onClicked: {
                            if (root.controller.setPassword(username.text, password.text)) {
                                username.clear()
                                password.clear()
                            }
                        }
                    }
                }
            }
        }

        GroupBox {
            title: qsTr("Display profiles")
            Layout.fillWidth: true
            ColumnLayout {
                anchors.fill: parent
                Repeater {
                    model: root.controller.profiles
                    delegate: RowLayout {
                        id: profileRow
                        required property string modelData
                        Layout.fillWidth: true
                        Label { text: profileRow.modelData; Layout.fillWidth: true }
                        Button { text: qsTr("Apply"); onClicked: root.controller.applyProfile(profileRow.modelData) }
                        Button { text: qsTr("Delete"); onClicked: root.controller.deleteProfile(profileRow.modelData) }
                    }
                }
                RowLayout {
                    TextField { id: profileName; placeholderText: qsTr("Profile name"); Layout.fillWidth: true }
                    Button {
                        text: qsTr("Save current layout")
                        enabled: profileName.text.length > 0
                        onClicked: if (root.controller.saveProfile(profileName.text)) profileName.clear()
                    }
                }
            }
        }
    }
}
