import QtQuick 2.6
import QtQuick.Layouts 1.3
import JAGCS 1.0

import "qrc:/Controls" as Controls

Item {
    id: missionList

    property alias missions: list.model

    onVisibleChanged: menu.filterEnabled = visible
    Component.onCompleted: menu.filterEnabled = true;

    Connections{
        target: menu
        onFilter: presenter.filter(text)
    }

    MissionListPresenter {
        id: presenter
        view: missionList
    }

    ListView {
        id: list
        anchors.fill: parent
        anchors.margins: sizings.shadowSize
        anchors.bottomMargin: addButton.height
        spacing: sizings.spacing
        flickableDirection: Flickable.AutoFlickIfNeeded
        boundsBehavior: Flickable.StopAtBounds

        Controls.ScrollBar.vertical: Controls.ScrollBar {
            visible: parent.contentHeight > parent.height
        }

        delegate: MissionView {
            width: parent.width
            anchors.horizontalCenter: parent.horizontalCenter
            missionId: model.missionId
        }
    }

    Controls.Label {
        anchors.centerIn: parent
        text: qsTr("No missions present")
        visible: list.count === 0
    }

    Controls.RoundButton {
        id: addButton
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        iconSource: "qrc:/ui/plus.svg"
        tipText: qsTr("Add Mission")
        onClicked: presenter.addMission(map.centerOffsetted)
    }
}
