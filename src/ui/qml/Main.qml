import QtQuick
import QtQuick.Controls
import org.kde.kheadless

ApplicationWindow {
    width: 1120
    height: 720
    minimumWidth: 840
    minimumHeight: 560
    visible: true
    title: qsTr("KHeadless")

    KHeadlessView {
        anchors.fill: parent
    }
}

