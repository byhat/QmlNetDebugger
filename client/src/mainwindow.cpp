#include "mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTimer>
#include <QDebug>
#include <QDateTime>
#include <QTemporaryFile>
#include <QFileInfo>
#include <QQuickWindow>
#include <QQuickItem>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_quickWidget(nullptr)
    , m_qmlEngine(nullptr)
    , m_qmlComponent(nullptr)
    , m_isLoading(false)
    , m_updateIndicatorVisible(false)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qInfo() << "[" << timestamp << "] MainWindow::MainWindow - Initializing MainWindow";
    
    // Initialize settings
    m_settings = new Settings();
    
    // Initialize network loader
    m_networkLoader = new QmlNetworkLoader(this);
    
    // Setup UI
    setupUi();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    
    // Restore window state
    if (!m_settings->windowGeometry().isEmpty()) {
        restoreGeometry(m_settings->windowGeometry());
    }
    if (!m_settings->windowState().isEmpty()) {
        restoreState(m_settings->windowState());
    }
    
    // Connect network loader signals
    connect(m_networkLoader, &QmlNetworkLoader::qmlLoaded,
            this, &MainWindow::onQmlLoaded);
    connect(m_networkLoader, &QmlNetworkLoader::qmlUnchanged,
            this, &MainWindow::onQmlUnchanged);
    connect(m_networkLoader, &QmlNetworkLoader::connectionStateChanged,
            this, &MainWindow::onConnectionStateChanged);
    connect(m_networkLoader, &QmlNetworkLoader::errorOccurred,
            this, &MainWindow::onErrorOccurred);
    connect(m_networkLoader, &QmlNetworkLoader::updateCheckCompleted,
            this, &MainWindow::onUpdateCheckCompleted);
    
    qInfo() << "[" << timestamp << "] MainWindow::MainWindow - MainWindow initialized, showing connection dialog in 100ms";
    
    // Show connection dialog on startup
    QTimer::singleShot(100, this, &MainWindow::showConnectionDialog);
}

MainWindow::~MainWindow()
{
    // Save window state
    m_settings->setWindowGeometry(saveGeometry());
    m_settings->setWindowState(saveState());
    m_settings->save();
    
    // Disconnect network loader
    m_networkLoader->disconnectFromServer();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Save window state before closing
    m_settings->setWindowGeometry(saveGeometry());
    m_settings->setWindowState(saveState());
    m_settings->save();
    
    QMainWindow::closeEvent(event);
}

void MainWindow::setupUi()
{
    setWindowTitle("QmlNetDebugger");
    setMinimumSize(800, 600);
    
    // Create central widget
    auto *centralWidget = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Create QML engine
    m_qmlEngine = new QQmlEngine(this);
    
    // Create quick widget for QML rendering
    m_quickWidget = new QQuickWidget(m_qmlEngine, this);
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_quickWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Connect QQuickWidget status changed signal to handle errors
    connect(m_quickWidget, &QQuickWidget::statusChanged,
            this, [this](QQuickWidget::Status status) {
                QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
                if (status == QQuickWidget::Error) {
                    qCritical() << "[" << timestamp << "] QQuickWidget status: Error";
                    for (const QQmlError &error : m_quickWidget->errors()) {
                        qCritical() << "[" << timestamp << "] QML Error at"
                                   << error.url().toString() << ":" << error.line() << "-" << error.description();
                    }
                }
            });
    
    // Create QML component (kept for potential future use)
    m_qmlComponent = new QQmlComponent(m_qmlEngine, this);
    connect(m_qmlComponent, &QQmlComponent::statusChanged,
            this, &MainWindow::onQmlComponentStatusChanged);
    
    mainLayout->addWidget(m_quickWidget);
    
    setCentralWidget(centralWidget);
}

void MainWindow::setupMenuBar()
{
    auto *menuBar = new QMenuBar(this);
    
    // File menu
    auto *fileMenu = menuBar->addMenu("&File");
    
    m_connectAction = new QAction("&Connect...", this);
    m_connectAction->setShortcut(QKeySequence::Open);
    m_connectAction->setStatusTip("Connect to QML server");
    connect(m_connectAction, &QAction::triggered, this, &MainWindow::showConnectionDialog);
    fileMenu->addAction(m_connectAction);
    
    m_disconnectAction = new QAction("&Disconnect", this);
    m_disconnectAction->setShortcut(QKeySequence::Close);
    m_disconnectAction->setEnabled(false);
    m_disconnectAction->setStatusTip("Disconnect from server");
    connect(m_disconnectAction, &QAction::triggered, this, &MainWindow::disconnect);
    fileMenu->addAction(m_disconnectAction);
    
    fileMenu->addSeparator();
    
    m_exitAction = new QAction("E&xit", this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setStatusTip("Exit application");
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(m_exitAction);
    
    // View menu
    auto *viewMenu = menuBar->addMenu("&View");
    
    m_refreshAction = new QAction("&Refresh", this);
    m_refreshAction->setShortcut(QKeySequence::Refresh);
    m_refreshAction->setEnabled(false);
    m_refreshAction->setStatusTip("Refresh QML content");
    connect(m_refreshAction, &QAction::triggered, this, &MainWindow::refresh);
    viewMenu->addAction(m_refreshAction);
    
    // Help menu
    auto *helpMenu = menuBar->addMenu("&Help");
    
    m_aboutAction = new QAction("&About", this);
    m_aboutAction->setStatusTip("About QmlNetDebugger");
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
    helpMenu->addAction(m_aboutAction);
    
    setMenuBar(menuBar);
}

void MainWindow::setupToolBar()
{
    auto *toolBar = addToolBar("Main");
    toolBar->setMovable(false);
    
    toolBar->addAction(m_connectAction);
    toolBar->addAction(m_disconnectAction);
    toolBar->addSeparator();
    toolBar->addAction(m_refreshAction);
}

void MainWindow::setupStatusBar()
{
    auto *statusBar = new QStatusBar(this);
    
    // Connection status label
    m_connectionStatusLabel = new QLabel("Not connected", this);
    statusBar->addWidget(m_connectionStatusLabel);
    
    // Update mode label
    m_updateModeLabel = new QLabel("", this);
    statusBar->addPermanentWidget(m_updateModeLabel);
    
    // Update time label
    m_updateTimeLabel = new QLabel("", this);
    statusBar->addPermanentWidget(m_updateTimeLabel);
    
    // Loading progress bar
    m_loadingProgressBar = new QProgressBar(this);
    m_loadingProgressBar->setMaximumWidth(150);
    m_loadingProgressBar->setTextVisible(false);
    m_loadingProgressBar->setRange(0, 0); // Indeterminate progress
    m_loadingProgressBar->hide();
    statusBar->addPermanentWidget(m_loadingProgressBar);
    
    // Refresh button
    m_refreshButton = new QPushButton("Refresh", this);
    m_refreshButton->setEnabled(false);
    connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::refresh);
    statusBar->addPermanentWidget(m_refreshButton);
    
    setStatusBar(statusBar);
}

void MainWindow::showConnectionDialog()
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qInfo() << "[" << timestamp << "] MainWindow::showConnectionDialog - Showing connection dialog";
    
    ConnectionDialog dialog(m_settings, this);
    
    if (dialog.exec() == QDialog::Accepted) {
        // Connect to server
        QUrl serverUrl = dialog.serverUrl();
        QString qmlFilename = dialog.qmlFilename();
        bool useSSE = dialog.useSSE();
        
        QString connectTimestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        qInfo() << "[" << connectTimestamp << "] MainWindow::showConnectionDialog - Connection accepted - URL:" << serverUrl.toString()
                << "QML file:" << qmlFilename << "Use SSE:" << useSSE;
        
        // Update network loader settings
        m_networkLoader->setUpdateInterval(dialog.updateInterval());
        
        // Connect
        if (m_networkLoader->connectToServer(serverUrl, qmlFilename, useSSE)) {
            m_isLoading = true;
            m_loadingProgressBar->show();
            updateStatusBar();
        } else {
            qCritical() << "[" << connectTimestamp << "] MainWindow::showConnectionDialog - Failed to initiate connection";
        }
    } else {
        qInfo() << "[" << timestamp << "] MainWindow::showConnectionDialog - Connection dialog cancelled";
    }
}

void MainWindow::disconnect()
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qInfo() << "[" << timestamp << "] MainWindow::disconnect - Disconnecting from server";
    
    m_networkLoader->disconnectFromServer();
    clearQmlView();
    updateStatusBar();
}

void MainWindow::refresh()
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qInfo() << "[" << timestamp << "] MainWindow::refresh - Refreshing QML content";
    
    m_networkLoader->refresh();
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, "About QmlNetDebugger",
                       "<h2>QmlNetDebugger</h2>"
                       "<p>Version 1.0.0</p>"
                       "<p>A Qt6 client for loading QML files over a network.</p>"
                       "<p>Features:</p>"
                       "<ul>"
                       "<li>Remote QML file loading</li>"
                       "<li>Hot-reload with ETag support</li>"
                       "<li>Server-Sent Events (SSE) for real-time updates</li>"
                       "<li>Cross-platform support</li>"
                       "</ul>");
}

void MainWindow::onQmlLoaded(const QString &content, const QString &etag)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qInfo() << "[" << timestamp << "] MainWindow::onQmlLoaded - QML content loaded - Size:" << content.size()
            << "bytes" << "ETag:" << etag;
    
    loadQmlContent(content);
    
    m_isLoading = false;
    m_loadingProgressBar->hide();
    
    // Flash update indicator
    m_updateIndicatorVisible = true;
    QTimer::singleShot(500, this, [this]() {
        m_updateIndicatorVisible = false;
        updateStatusBar();
    });
    
    updateStatusBar();
}

void MainWindow::onQmlUnchanged()
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qInfo() << "[" << timestamp << "] MainWindow::onQmlUnchanged - QML content unchanged";
    
    // QML content unchanged, no action needed
    updateStatusBar();
}

void MainWindow::onConnectionStateChanged(QmlNetworkLoader::ConnectionState state)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString stateStr;
    switch (state) {
        case QmlNetworkLoader::ConnectionState::Disconnected:
            stateStr = "Disconnected";
            m_connectAction->setEnabled(true);
            m_disconnectAction->setEnabled(false);
            m_refreshAction->setEnabled(false);
            m_refreshButton->setEnabled(false);
            break;
            
        case QmlNetworkLoader::ConnectionState::Connecting:
            stateStr = "Connecting";
            m_connectAction->setEnabled(false);
            m_disconnectAction->setEnabled(true);
            m_refreshAction->setEnabled(false);
            m_refreshButton->setEnabled(false);
            break;
            
        case QmlNetworkLoader::ConnectionState::Connected:
            stateStr = "Connected";
            m_connectAction->setEnabled(false);
            m_disconnectAction->setEnabled(true);
            m_refreshAction->setEnabled(true);
            m_refreshButton->setEnabled(true);
            break;
            
        case QmlNetworkLoader::ConnectionState::Error:
            stateStr = "Error";
            m_connectAction->setEnabled(true);
            m_disconnectAction->setEnabled(true);
            m_refreshAction->setEnabled(false);
            m_refreshButton->setEnabled(false);
            break;
    }
    
    qInfo() << "[" << timestamp << "] MainWindow::onConnectionStateChanged - Connection state changed to:" << stateStr;
    
    updateStatusBar();
}

void MainWindow::onErrorOccurred(const QString &error)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qCritical() << "[" << timestamp << "] MainWindow::onErrorOccurred - Error occurred:" << error;
    
    showError(error);
    
    if (m_isLoading) {
        m_isLoading = false;
        m_loadingProgressBar->hide();
    }
    
    updateStatusBar();
}

void MainWindow::onUpdateCheckCompleted(bool updated)
{
    Q_UNUSED(updated)
    // Status bar is updated by other handlers
}

void MainWindow::onQmlComponentStatusChanged(QQmlComponent::Status status)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    
    switch (status) {
        case QQmlComponent::Ready: {
            // Component loaded successfully
            qInfo() << "[" << timestamp << "] MainWindow::onQmlComponentStatusChanged - QML component status: Ready";
            
            // Clear existing content before setting new content
            // This prevents old QML objects from piling up on top of each other
            m_quickWidget->setSource(QUrl());
            
            // Create the QML object from the component data
            QObject *qmlObject = m_qmlComponent->create();
            if (qmlObject) {
                qInfo() << "[" << timestamp << "] MainWindow::onQmlComponentStatusChanged - QML object created successfully"
                        << "Address:" << qmlObject << "Parent:" << qmlObject->parent()
                        << "ClassName:" << qmlObject->metaObject()->className();
                
                // Use setContent() to set the QML object
                // Since we're now using Item instead of ApplicationWindow, this won't create new windows
                m_quickWidget->setContent(QUrl(), m_qmlComponent, qmlObject);
                
                qInfo() << "[" << timestamp << "] MainWindow::onQmlComponentStatusChanged - DEBUG: After loading, new root object:"
                        << m_quickWidget->rootObject();
            } else {
                qCritical() << "[" << timestamp << "] MainWindow::onQmlComponentStatusChanged - Failed to create QML object";
            }
            break;
        }
            
        case QQmlComponent::Error: {
            // Component has errors
            qCritical() << "[" << timestamp << "] MainWindow::onQmlComponentStatusChanged - QML component status: Error";
            
            QStringList errors;
            for (const QQmlError &error : m_qmlComponent->errors()) {
                qCritical() << "[" << timestamp << "] MainWindow::onQmlComponentStatusChanged - QML Error at"
                           << error.url().toString() << ":" << error.line() << "-" << error.description();
                errors << QString("%1:%2 - %3")
                              .arg(error.url().toString())
                              .arg(error.line())
                              .arg(error.description());
            }
            showError("QML Error:\n" + errors.join("\n"));
            break;
        }
            
        case QQmlComponent::Loading:
            // Component is loading
            qInfo() << "[" << timestamp << "] MainWindow::onQmlComponentStatusChanged - QML component status: Loading";
            break;
    }
}

void MainWindow::loadQmlContent(const QString &content)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qInfo() << "[" << timestamp << "] MainWindow::loadQmlContent - Loading QML content, size:" << content.size() << "bytes";
    
    // DIAGNOSTIC: Check current root object before clearing
    QQuickItem *oldRootObject = m_quickWidget->rootObject();
    qInfo() << "[" << timestamp << "] MainWindow::loadQmlContent - Old root object:" << oldRootObject
            << "Address:" << (quintptr)oldRootObject;
    
    // Clear existing content first
    m_quickWidget->setSource(QUrl());
    
    // DIAGNOSTIC: Check root object after clearing
    QQuickItem *clearedRootObject = m_quickWidget->rootObject();
    qInfo() << "[" << timestamp << "] MainWindow::loadQmlContent - Root object after clear:" << clearedRootObject
            << "Address:" << (quintptr)clearedRootObject;
    
    // DIAGNOSTIC: Check QML engine component cache
    qInfo() << "[" << timestamp << "] MainWindow::loadQmlContent - QML engine address:" << m_qmlEngine;
    
    // Create a persistent file to store the QML content
    // We use a fixed filename instead of a temporary file to avoid deletion issues
    QString filePath = "/tmp/qmlnetdebugger.qml";
    QFile *qmlFile = new QFile(filePath, this);
    if (!qmlFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCritical() << "[" << timestamp << "] MainWindow::loadQmlContent - Failed to create QML file";
        return;
    }
    qmlFile->write(content.toUtf8());
    qmlFile->flush();
    qmlFile->close();
    
    // DIAGNOSTIC: Verify file content was written
    QFile verifyFile(filePath);
    if (verifyFile.open(QIODevice::ReadOnly)) {
        QByteArray fileContent = verifyFile.readAll();
        qInfo() << "[" << timestamp << "] MainWindow::loadQmlContent - File verification - written:" << content.size()
                << "bytes, file contains:" << fileContent.size() << "bytes, match:" << (content.toUtf8() == fileContent);
        verifyFile.close();
    }
    
    // Get the file URL
    QUrl fileUrl = QUrl::fromLocalFile(filePath);
    qInfo() << "[" << timestamp << "] MainWindow::loadQmlContent - Created QML file:" << filePath << "URL:" << fileUrl;
    
    // Load the QML content using setSource with the file URL
    // Since we're using Item instead of ApplicationWindow, this will work correctly
    m_quickWidget->setSource(fileUrl);
    
    // DIAGNOSTIC: Check root object after loading
    QQuickItem *newRootObject = m_quickWidget->rootObject();
    qInfo() << "[" << timestamp << "] MainWindow::loadQmlContent - New root object:" << newRootObject
            << "Address:" << (quintptr)newRootObject
            << "Same as old:" << (newRootObject == oldRootObject);
    
    // DIAGNOSTIC: Check if source changed
    qInfo() << "[" << timestamp << "] MainWindow::loadQmlContent - Source URL:" << m_quickWidget->source();
}

void MainWindow::clearQmlView()
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qInfo() << "[" << timestamp << "] MainWindow::clearQmlView - Clearing QML view";
    
    m_quickWidget->setSource(QUrl());
}

void MainWindow::showError(const QString &error)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qCritical() << "[" << timestamp << "] MainWindow::showError - Showing error dialog:" << error;
    
    QMessageBox::critical(this, "Error", error);
}

void MainWindow::showInfo(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qInfo() << "[" << timestamp << "] MainWindow::showInfo - Showing info dialog:" << message;
    
    QMessageBox::information(this, "Information", message);
}

void MainWindow::updateStatusBar()
{
    // Update connection status
    QString stateText = connectionStateToString(m_networkLoader->connectionState());
    m_connectionStatusLabel->setText(stateText);
    
    // Update update mode - Note: The mode is determined by what was requested in the connection dialog
    // We need to track this separately since both SSE and polling use ETags
    // For now, we'll show "Connected" since we can't directly check SSE state from public API
    if (m_networkLoader->connectionState() == QmlNetworkLoader::ConnectionState::Connected) {
        m_updateModeLabel->setText(QString("Mode: Connected"));
    } else {
        m_updateModeLabel->clear();
    }
    
    // Update time
    if (!m_networkLoader->lastUpdateTime().isNull()) {
        QString timeText = m_networkLoader->lastUpdateTime().toString("HH:mm:ss");
        if (m_updateIndicatorVisible) {
            timeText += " (Updated)";
        }
        m_updateTimeLabel->setText(QString("Last update: %1").arg(timeText));
    } else {
        m_updateTimeLabel->clear();
    }
    
    // Update loading indicator
    if (m_isLoading) {
        m_loadingProgressBar->show();
    } else {
        m_loadingProgressBar->hide();
    }
}

QString MainWindow::connectionStateToString(QmlNetworkLoader::ConnectionState state) const
{
    switch (state) {
        case QmlNetworkLoader::ConnectionState::Disconnected:
            return "Not connected";
        case QmlNetworkLoader::ConnectionState::Connecting:
            return "Connecting...";
        case QmlNetworkLoader::ConnectionState::Connected:
            return "Connected";
        case QmlNetworkLoader::ConnectionState::Error:
            return "Connection error";
    }
    return "Unknown";
}
