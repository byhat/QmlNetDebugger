#include <QApplication>
#include <QIcon>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application information
    app.setApplicationName("QmlNetDebugger");
    app.setApplicationDisplayName("QmlNetDebugger");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("QmlNetDebugger");
    app.setOrganizationDomain("qmlnetdebugger.com");

    MainWindow window;
    window.show();

    return app.exec();
}
