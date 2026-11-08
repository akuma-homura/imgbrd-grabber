import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12

Dialog {
    id: root

    title: qsTr("Batch download")
    modal: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(400, Overlay.overlay.width - 32)
    standardButtons: Dialog.Ok | Dialog.Cancel

    signal startRequested(bool currentPage, int limit, bool byArtist, bool originals)

    onAccepted: startRequested(currentPage.checked, limit.value,
                               byArtist.checked, originals.checked)

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Label {
            text: qsTr("Download range")
            font.bold: true
        }
        RadioButton {
            id: currentPage
            text: qsTr("Current search page")
            checked: true
        }
        RadioButton {
            id: allPages
            text: qsTr("Across search pages")
        }
        RowLayout {
            enabled: allPages.checked
            Label { text: qsTr("Maximum images") }
            SpinBox {
                id: limit
                from: 1
                to: 10000
                value: 500
                editable: true
                Layout.fillWidth: true
            }
        }
        CheckBox {
            id: byArtist
            text: qsTr("Create folders by artist tags")
            checked: true
        }
        CheckBox {
            id: originals
            text: qsTr("Download original files")
            checked: true
        }
        Label {
            text: qsTr("Files: Pictures/Grabber/<site>/<artist>/")
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            font.pixelSize: 12
        }
    }
}
