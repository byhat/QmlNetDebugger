#pragma once

#include <QMainWindow>
#include <QQuickWidget>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QStatusBar>
#include <QLabel>
#include <QPushButton>
#include <QAction>
#include <QMenuBar>
#include <QToolBar>
#include <QMessageBox>
#include <QTimer>
#include <QProgressBar>

#include "settings.h"
#include "qmlnetworkloader.h"
#include "connectiondialog.h"

/**
 * @brief Main application window
 * 
 * Hosts the QML view and manages the application lifecycle.
 * Handles menu actions, status bar updates, and error display.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit MainWindow(QWidget *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~MainWindow() override;

protected:
    /**
     * @brief Handle window close event
     * @param event Close event
     */
    void closeEvent(QCloseEvent *event) override;

private slots:
    void showConnectionDialog();
    void disconnect();
    void refresh();
    void showAbout();
    void onQmlLoaded(const QString &content, const QString &etag);
    void onQmlUnchanged();
    void onConnectionStateChanged(QmlNetworkLoader::ConnectionState state);
    void onErrorOccurred(const QString &error);
    void onUpdateCheckCompleted(bool updated);
    void onQmlComponentStatusChanged(QQmlComponent::Status status);
    void updateStatusBar();

    /**
     * @brief Handle bundle downloaded — load QML from local folder
     * @param qmlDir Local path to the qml folder
     */
    void onBundleDownloaded(const QString &qmlDir);

    /**
     * @brief Handle single file updated via SSE
     * @param relativePath Relative path of the updated file
     */
    void onFileUpdated(const QString &relativePath);

private:
    void setupUi();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void loadQmlContent(const QString &content);
    void loadQmlFromFolder(const QString &qmlDir);
    void clearQmlView();
    void showError(const QString &error);
    void showInfo(const QString &message);
    QString connectionStateToString(QmlNetworkLoader::ConnectionState state) const;

    // UI Components
    QQuickWidget *m_quickWidget;
    QQmlEngine *m_qmlEngine;
    QQmlComponent *m_qmlComponent;
    
    // Status bar components
    QLabel *m_connectionStatusLabel;
    QLabel *m_updateTimeLabel;
    QLabel *m_updateModeLabel;
    QProgressBar *m_loadingProgressBar;
    QPushButton *m_refreshButton;
    
    // Actions
    QAction *m_connectAction;
    QAction *m_disconnectAction;
    QAction *m_refreshAction;
    QAction *m_exitAction;
    QAction *m_aboutAction;
    
    // Application components
    Settings *m_settings;
    QmlNetworkLoader *m_networkLoader;
    
    // State
    bool m_isLoading;
    bool m_updateIndicatorVisible;
    QString m_qmlDir;
};

