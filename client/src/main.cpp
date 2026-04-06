#include <QApplication>
#include <QIcon>

#include "mainwindow.h"

/**
 * @brief Main entry point for QmlNetDebugger client
 * 
 * Initializes the Qt application and creates the main window.
 */
int main(int argc, char *argv[])
{
    // Create Qt application
    QApplication app(argc, argv);
    
    // Set application information
    app.setApplicationName("QmlNetDebugger");
    app.setApplicationDisplayName("QmlNetDebugger");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("QmlNetDebugger");
    app.setOrganizationDomain("qmlnetdebugger.com");
    
    // Create and show main window
    MainWindow window;
    window.show();
    
    // Run application
    return app.exec();
}
