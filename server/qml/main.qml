import QtQml
import QtQuick
import QtCore
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts
// import Qt.labs.settings

import "./elements"


Item
{
    id: main

    x: 10
    y: 10

    width: 1780
    height: 850

    visible: true

    //title: qsTr("QAnalyser")

    //color: "white"

    QtObject {
        id: appTemp
        property var tempConfig: ({})  // хранение настроек для передачи между страницами
    }


    Settings {
        property alias x: main.x
        property alias y: main.y
        property alias width: main.width
        property alias height: main.height
    }

    FontLoader {
        id: montserratBold;
        source: "./fonts/Montserrat/static/Montserrat-SemiBold.ttf"
    }

    MessageDialog {
        id: msgDialog
        anchors.centerIn: parent
    }

    Rectangle {
        id: choosePageBg

        width: 160
        height: parent.height

        color: "#e1e1e1"

        Rectangle { // правая граница
            width: 2
            height: parent.height
            color: "black"
            anchors.left: parent.right
        }

        ListView {
            id: choosePageView

            width: parent.width
            height: parent.height - 50

            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter:   parent.verticalCenter

            //spacing:  10
            model: choosePageLst
            delegate:

                ChoosePageBtn {
                    width: choosePageBg.width
                    height: choosePageBg.width * 1.2

                    anchors.horizontalCenter: parent.horizontalCenter

                    imagePath: model.imagePath
                    btnText:   model.btnText

                    onBtnClicked: {
                        loader.loadFragment(model.index);
                        choosePageView.currentIndex = model.index
                    }

                    blurVisible: choosePageView.currentIndex === model.index
                }
        }
    }

    ListModel {
        id: choosePageLst

        ListElement {
            imagePath: "../images/home.svg"
            btnText:   "Главная\nстраница"
        }

        ListElement {
            imagePath: "../images/setup.svg"
            btnText:   "Настройки"
        }

        ListElement {
            imagePath: "../images/info.svg"
            btnText:   "О программе"
        }
    }

    Loader {
        id: loader

        width: parent.width - choosePageView.width
        height: parent.height

        anchors.left: choosePageBg.right

        source: "./pages/FirstPage.qml"

        function loadFragment(index) {
            switch(index) {
            case 0:
                loader.source = "./pages/FirstPage.qml"
                break;
            case 1:
                loader.source = "./pages/ConfigPage.qml"
                break;
            case 2:
                loader.source = "./pages/InfoPage.qml"
                break;
            default:
                loader.source = "./pages/FirstPage.qml"
                break;
            }
        }
    }

    // Connections {
    //     target: app
    //     function onQmlMessageUpdate(message) {
    //         msgDialog.message = message.text
    //         msgDialog.open()
    //     }
    // }

    Component.onCompleted: app.start()
}
