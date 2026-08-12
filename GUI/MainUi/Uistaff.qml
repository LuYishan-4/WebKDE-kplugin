import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import QtQuick.Dialogs

Item {
    id: root

    property var backend

    property color bgColor: "#0d0d14"
    property color sidebarColor: "#15151f"
    property color accentColor: "#89b4fa"
    property color cardColor: "#1e1e2e"
    property color cardHoverColor: "#282838"
    property color textColor: "#cdd6f4"
    property color subTextColor: "#7f849c"
    property color dangerColor: "#f38ba8"

    property string searchQuery: ""

    FolderDialog {
        id: themeUploadDialog
        title: qsTr("Choose Theme Folder")
        onAccepted: root.backend.uploadTheme(selectedFolder.toString().replace("file://", ""))
    }

    Rectangle {
        anchors.fill: parent
        color: bgColor
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ---------------- Sidebar ----------------
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 200
            color: sidebarColor

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 4

                Label {
                    text: "WEBCURSOR"
                    font.pixelSize: 20
                    font.bold: true
                    font.letterSpacing: 1
                    color: accentColor
                    Layout.alignment: Qt.AlignHCenter
                    Layout.bottomMargin: 24
                }

                ButtonGroup { id: navGroup }

                SidebarButton {
                    text: qsTr("Gallery")
                    icon: "🖼"
                    checked: true
                    ButtonGroup.group: navGroup
                    onClicked: stackLayout.currentIndex = 0
                }
                SidebarButton {
                    text: qsTr("Blacklist")
                    icon: "🚫"
                    ButtonGroup.group: navGroup
                    onClicked: stackLayout.currentIndex = 1
                }
                SidebarButton {
                    text: qsTr("Settings")
                    icon: "⚙"
                    ButtonGroup.group: navGroup
                    onClicked: stackLayout.currentIndex = 2
                }

                Item { Layout.fillHeight: true }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    radius: 8
                    color: root.backend && root.backend.enabled ? "#2a3a2a" : "#3a2a2a"

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        Rectangle {
                            width: 8; height: 8; radius: 4
                            color: root.backend && root.backend.enabled ? "#a6e3a1" : "#f38ba8"
                        }
                        Label {
                            text: root.backend && root.backend.enabled ? qsTr("Running") : qsTr("Stopped")
                            color: textColor
                            font.pixelSize: 12
                            Layout.fillWidth: true
                        }
                    }
                }

                Label {
                    text: root.backend ? root.backend.statusMessage : ""
                    color: subTextColor
                    font.pixelSize: 10
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    Layout.topMargin: 4
                }
            }
        }

        // ---------------- Main content ----------------
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: bgColor

            StackLayout {
                id: stackLayout
                anchors.fill: parent
                currentIndex: 0

                // ===== 1. Gallery (Wallpaper-Engine style) =====
                ColumnLayout {
                    spacing: 0

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.margins: 24
                        Layout.bottomMargin: 12
                        spacing: 12

                        Label {
                            text: qsTr("Cursor Gallery")
                            font.pixelSize: 24
                            font.bold: true
                            color: textColor
                        }

                        Item { Layout.fillWidth: true }

                        Rectangle {
                            Layout.preferredWidth: 260
                            Layout.preferredHeight: 36
                            radius: 18
                            color: cardColor
                            border.color: searchField.activeFocus ? accentColor : "transparent"
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 14
                                anchors.rightMargin: 10
                                spacing: 6

                                Label { text: "🔍"; color: subTextColor; font.pixelSize: 13 }
                                TextField {
                                    id: searchField
                                    Layout.fillWidth: true
                                    placeholderText: qsTr("Search cursors...")
                                    color: textColor
                                    background: null
                                    onTextChanged: root.searchQuery = text
                                }
                            }
                        }

                        Button {
                            text: qsTr("+ Import")
                            onClicked: themeUploadDialog.open()
                        }
                    }

                    GridView {
                        id: themeGrid
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.margins: 24
                        Layout.topMargin: 0
                        cellWidth: 220; cellHeight: 240
                        clip: true

                        model: {
                            var all = root.backend ? root.backend.themeList : [];
                            if (root.searchQuery.trim().length === 0)
                                return all;
                            var q = root.searchQuery.toLowerCase();
                            return all.filter(function (name) {
                                return name.toLowerCase().indexOf(q) !== -1;
                            });
                        }

                        delegate: Item {
                            id: cardRoot
                            width: themeGrid.cellWidth - 16
                            height: themeGrid.cellHeight - 16

                            property var details: (modelData !== undefined && modelData !== "") ? root.backend.getThemeDetails(modelData) : null
                            property bool isCurrent: root.backend.currentTheme === modelData
                            property bool hovered: cardMouse.containsMouse

                            Rectangle {
                                id: card
                                anchors.fill: parent
                                radius: 12
                                color: cardRoot.hovered ? cardHoverColor : cardColor
                                border.color: cardRoot.isCurrent ? accentColor : "transparent"
                                border.width: 2

                                Behavior on color { ColorAnimation { duration: 120 } }

                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: 0

                                    // Icon / thumbnail area
                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 150
                                        radius: 12
                                        color: "#000000"
                                        clip: true

                                        Image {
                                            anchors.fill: parent
                                            anchors.margins: 8
                                            source: cardRoot.details ? (cardRoot.details.iconPath || "") : ""
                                            fillMode: Image.PreserveAspectFit
                                            visible: source != ""
                                        }

                                        Label {
                                            anchors.centerIn: parent
                                            visible: !cardRoot.details || !cardRoot.details.iconPath
                                            text: "🖱"
                                            font.pixelSize: 40
                                            opacity: 0.3
                                        }

                                        Rectangle {
                                            visible: cardRoot.isCurrent
                                            anchors.top: parent.top
                                            anchors.right: parent.right
                                            anchors.margins: 8
                                            width: 22; height: 22; radius: 11
                                            color: accentColor
                                            Label {
                                                anchors.centerIn: parent
                                                text: "✓"
                                                color: "#11111b"
                                                font.bold: true
                                                font.pixelSize: 12
                                            }
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.margins: 10
                                        spacing: 2

                                        Label {
                                            text: modelData
                                            font.bold: true
                                            font.pixelSize: 13
                                            color: textColor
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                        Label {
                                            text: cardRoot.details ? (qsTr("by ") + (cardRoot.details.author || qsTr("Unknown"))) : ""
                                            font.pixelSize: 11
                                            color: subTextColor
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                    }
                                }

                                // Hover overlay with actions
                                Rectangle {
                                    anchors.fill: parent
                                    radius: 12
                                    color: "#000000"
                                    opacity: cardRoot.hovered ? 0.55 : 0
                                    Behavior on opacity { NumberAnimation { duration: 120 } }
                                }

                                RowLayout {
                                    anchors.centerIn: parent
                                    spacing: 8
                                    opacity: cardRoot.hovered ? 1 : 0
                                    Behavior on opacity { NumberAnimation { duration: 120 } }

                                    Button {
                                        text: cardRoot.isCurrent ? qsTr("In Use") : qsTr("Apply")
                                        enabled: !cardRoot.isCurrent
                                        onClicked: root.backend.useTheme(modelData)
                                    }
                                    Button {
                                        text: qsTr("Remove")
                                        onClicked: removeConfirmDialog.openFor(modelData)
                                    }
                                }

                                MouseArea {
                                    id: cardMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                }
                            }
                        }
                    }
                }

                // ===== 2. Blacklist =====
                ColumnLayout {
                    spacing: 20
                    Layout.margins: 24

                    Label { text: qsTr("Blacklisted Applications"); font.pixelSize: 22; font.bold: true; color: textColor }
                    Label {
                        text: qsTr("The custom cursor is hidden while any of these application windows are focused.")
                        color: subTextColor
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        TextField {
                            id: blacklistInput
                            Layout.fillWidth: true
                            placeholderText: qsTr("Window class, e.g. steam_app_12345")
                            onAccepted: addBlacklistEntry()
                        }
                        Button {
                            text: qsTr("Add")
                            enabled: blacklistInput.text.trim().length > 0
                            onClicked: addBlacklistEntry()
                        }
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: root.backend.blacklist
                        spacing: 6

                        delegate: Rectangle {
                            width: ListView.view.width
                            height: 44
                            radius: 8
                            color: cardColor

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                spacing: 8

                                Label {
                                    text: modelData
                                    color: textColor
                                    Layout.fillWidth: true
                                    elide: Text.ElideMiddle
                                }
                                Button {
                                    text: qsTr("Remove")
                                    onClicked: root.backend.removeBlacklist(modelData)
                                }
                            }
                        }

                        Label {
                            anchors.centerIn: parent
                            visible: parent.count === 0
                            text: qsTr("No applications blacklisted")
                            color: subTextColor
                        }
                    }

                    function addBlacklistEntry() {
                        var value = blacklistInput.text.trim();
                        if (value.length === 0)
                            return;
                        root.backend.addBlacklist(value);
                        blacklistInput.text = "";
                    }
                }

                // ===== 3. Settings =====
                ColumnLayout {
                    spacing: 24
                    Layout.margins: 24

                    Label { text: qsTr("Settings"); font.pixelSize: 22; font.bold: true; color: textColor }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: settingsGrid.implicitHeight + 32
                        radius: 12
                        color: cardColor

                        GridLayout {
                            id: settingsGrid
                            anchors.fill: parent
                            anchors.margins: 16
                            columns: 2
                            rowSpacing: 20
                            columnSpacing: 24

                            Label { text: qsTr("Cursor Width"); color: textColor }
                            SpinBox {
                                from: 16; to: 1920
                                value: root.backend.cursorWidth
                                onValueModified: root.backend.cursorWidth = value
                            }

                            Label { text: qsTr("Cursor Height"); color: textColor }
                            SpinBox {
                                from: 16; to: 1920
                                value: root.backend.cursorHeight
                                onValueModified: root.backend.cursorHeight = value
                            }

                            Label { text: qsTr("Enable GPU Rendering"); color: textColor }
                            Switch {
                                checked: root.backend.gpuRender
                                onCheckedChanged: root.backend.gpuRender = checked
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }

                    RowLayout {
                        spacing: 12
                        Button {
                            text: root.backend.enabled ? qsTr("Stop Engine") : qsTr("Start Engine")
                            onClicked: root.backend.enabled ? root.backend.disable() : root.backend.enable()
                        }
                        Label {
                            text: root.backend.mainProcessConnected ? qsTr("Connected") : qsTr("Not connected")
                            color: root.backend.mainProcessConnected ? "#a6e3a1" : dangerColor
                            font.pixelSize: 12
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: removeConfirmDialog
        title: qsTr("Remove Theme")
        modal: true
        standardButtons: Dialog.Yes | Dialog.No
        anchors.centerIn: parent

        property string themeName: ""

        function openFor(name) {
            themeName = name;
            open();
        }

        Label {
            text: qsTr("Remove theme \"%1\"? This cannot be undone.").arg(removeConfirmDialog.themeName)
            wrapMode: Text.WordWrap
        }

        onAccepted: root.backend.removeTheme(themeName)
    }

    component SidebarButton: Button {
        id: sbBtn
        property string icon: ""
        Layout.fillWidth: true
        Layout.preferredHeight: 38
        checkable: true
        flat: true

        contentItem: RowLayout {
            spacing: 10
            anchors.fill: parent
            anchors.leftMargin: 12
            Label { text: sbBtn.icon; font.pixelSize: 15 }
            Label {
                text: sbBtn.text
                color: root.textColor
                font.pixelSize: 13
                Layout.fillWidth: true
            }
        }

        background: Rectangle {
            radius: 8
            color: sbBtn.checked ? "#2a2a3a" : (sbBtn.hovered ? "#20202c" : "transparent")
            Behavior on color { ColorAnimation { duration: 100 } }
        }
    }
}
