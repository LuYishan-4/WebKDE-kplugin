import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material // 可選，統一兩邊的視覺風格

import "../MainUi" // 引入你的核心 UI

ApplicationWindow {
    id: window
    visible: true
    width: 1024
    height: 720
    title: qsTr("WebCursor")

    Material.theme: Material.Dark

    Uistaff {
        anchors.fill: parent
        backend: appBackend
    }
}
