import QtQuick 2.12
import QtQuick.Controls 2.12

ApplicationWindow {
    width: 800
    height: 480
    visible: true
    title: "IVI Demo"

    Column {
        anchors.centerIn: parent
        spacing: 20

        Button { text: "Media"; onClicked: console.log("Media clicked") }
        Button { text: "Bluetooth"; onClicked: console.log("BT clicked") }
        Button { text: "Settings"; onClicked: console.log("Settings clicked") }
    }
}
