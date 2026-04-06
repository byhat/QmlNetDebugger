import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

/**
 * @brief QML-based connection dialog
 * 
 * This is a placeholder QML file for the connection dialog.
 * The actual dialog is implemented in C++ using Qt Widgets for
 * better integration with the native platform look and feel.
 */
Item {
    id: root
    
    width: 400
    height: 300
    
    // This file is included in the resources but not currently used
    // The connection dialog is implemented in C++ (ConnectionDialog.cpp)
    
    Label {
        anchors.centerIn: parent
        text: "Connection dialog is implemented in C++"
        font.pixelSize: 14
        color: "#666666"
    }
}
