import QtQuick
import org.kde.kcmutils as KCM

import "../../MainUi"
KCM.SimpleKCM {
    id: root
    title: qsTr("WebCursor")

    Uistaff {
        anchors.fill: parent
        backend: kcm.backend
    }
}
