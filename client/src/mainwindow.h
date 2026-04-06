#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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
    /**
     * @brief Show connection dialog
     */
    void showConnectionDialog();

    /**
     * @brief Disconnect from server
     */
    void disconnect();

    /**
     * @brief Refresh QML content
     */
    void refresh();

    /**
     * @brief Show about dialog
     */
    void showAbout();

    /**
     * @brief Handle QML loaded event
     * @param content QML content
     * @param etag ETag of the content
     */
    void onQmlLoaded(const QString &content, const QString &etag);

    /**
     * @brief Handle QML unchanged event
     */
    void onQmlUnchanged();

    /**
     * @brief Handle connection state changed event
     * @param state New connection state
     */
    void onConnectionStateChanged(QmlNetworkLoader::ConnectionState state);

    /**
     * @brief Handle error occurred event
     * @param error Error message
     */
    void onErrorOccurred(const QString &error);

    /**
     * @brief Handle update check completed event
     * @param updated True if content was updated
     */
    void onUpdateCheckCompleted(bool updated);

    /**
     * @brief Handle QML component status change
     * @param status Component status
     */
    void onQmlComponentStatusChanged(QQmlComponent::Status status);

    /**
     * @brief Update status bar
     */
    void updateStatusBar();

private:
    /**
     * @brief Setup UI components
     */
    void setupUi();

    /**
     * @brief Setup menu bar
     */
    void setupMenuBar();

    /**
     * @brief Setup tool bar
     */
    void setupToolBar();

    /**
     * @brief Setup status bar
     */
    void setupStatusBar();

    /**
     * @brief Load QML content into the view
     * @param content QML content
     */
    void loadQmlContent(const QString &content);

    /**
     * @brief Clear QML view
     */
    void clearQmlView();

    /**
     * @brief Show error message
     * @param error Error message
     */
    void showError(const QString &error);

    /**
     * @brief Show info message
     * @param message Info message
     */
    void showInfo(const QString &message);

    /**
     * @brief Get connection state string
     * @param state Connection state
     * @return State string
     */
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
};

#endif // MAINWINDOW_H
