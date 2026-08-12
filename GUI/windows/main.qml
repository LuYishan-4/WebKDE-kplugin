import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

import "../MainUi"
ApplicationWindow {
    visible: true
    width: 1024
    height: 720
    title: qsTr("Ultralight_WebCursor")

    Material.theme: Material.Dark
    Material.accent: Material.DeepPurple

    Uistaff {
        anchors.fill: parent
        backend: appBackend
    }
}
