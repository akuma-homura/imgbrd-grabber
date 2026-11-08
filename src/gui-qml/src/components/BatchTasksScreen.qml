import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12

Page {
    id: root
    signal closed()

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            ToolButton {
                icon.source: "/images/icons/back.png"
                onClicked: root.closed()
            }
            Label {
                text: qsTr("Batch downloads")
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 14

        Label {
            text: batchController.status
            font.bold: true
            font.pixelSize: 20
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }
        ProgressBar {
            from: 0
            to: Math.max(1, batchController.total)
            value: batchController.processed
            Layout.fillWidth: true
        }
        Label {
            text: qsTr("%1 / %2 processed").arg(batchController.processed).arg(batchController.total)
            Layout.fillWidth: true
        }
        Label {
            text: qsTr("Recovered staged files: %1").arg(batchController.recovered)
            Layout.fillWidth: true
        }
        Label {
            text: qsTr("Saved: %1    Skipped: %2    Failed: %3")
                .arg(batchController.saved).arg(batchController.skipped).arg(batchController.failed)
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }
        Label {
            text: batchController.error
            visible: batchController.error.length > 0
            color: "tomato"
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }
        RowLayout {
            Layout.fillWidth: true
            Button {
                text: qsTr("Cancel")
                enabled: batchController.running
                onClicked: batchController.cancel()
            }
            Button {
                text: qsTr("Resume last task")
                enabled: !batchController.running && batchController.resumable
                onClicked: batchController.resumeLast()
            }
        }
        Label {
            text: qsTr("Exports use Android's public Pictures/Grabber directory. Closing the app interrupts an active task; reopen and choose Resume.")
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }
        Item { Layout.fillHeight: true }
    }
}
