import QtQuick 2.15
import QtQuick.Controls 2.15

Button {
    background: Rectangle {
        color: parent.pressed ? "#d0d0d0" : "#e0e0e0"
        border.color: "#a0a0a0"
        border.width: 1
        radius: 4
    }
    
    contentItem: Text {
        text: parent.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: "#000000"
    }
}
