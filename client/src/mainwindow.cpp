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
#include <QResizeEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_quickWidget(nullptr)
    , m_qmlEngine(nullptr)
    , m_qmlComponent(nullptr)
    , m_connectionDialog(nullptr)
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
    connect(m_networkLoader, &QmlNetworkLoader::bundleDownloaded,
            this, &MainWindow::onBundleDownloaded);
    connect(m_networkLoader, &QmlNetworkLoader::fileUpdated,
            this, &MainWindow::onFileUpdated);
    
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

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    
    // Keep the inline connection dialog overlay sized to the central widget
    if (m_connectionDialog && m_connectionDialog->isVisible()) {
        m_connectionDialog->setGeometry(m_centralWidget->rect());
    }
}

void MainWindow::setupUi()
{
    setWindowTitle("QmlNetDebugger");
    
    // Create central widget
    m_centralWidget = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(m_centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Create QML engine
    m_qmlEngine = new QQmlEngine(this);
    
    // Create quick widget for QML rendering
    m_quickWidget = new QQuickWidget(m_qmlEngine, m_centralWidget);
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
    
    // Create inline connection dialog overlay (child of central widget)
    m_connectionDialog = new ConnectionDialog(m_settings, m_centralWidget);
    connect(m_connectionDialog, &ConnectionDialog::accepted,
            this, &MainWindow::onConnectionDialogAccepted);
    connect(m_connectionDialog, &ConnectionDialog::rejected,
            this, &MainWindow::onConnectionDialogRejected);
    
    setCentralWidget(m_centralWidget);
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
    qInfo() << "[" << timestamp << "] MainWindow::showConnectionDialog - Showing inline connection dialog";
    
    m_connectionDialog->showDialog();
}

void MainWindow::onConnectionDialogAccepted()
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    
    // Connect to server using the dialog's current values
    QUrl serverUrl = m_connectionDialog->serverUrl();
    QString qmlFilename = m_connectionDialog->qmlFilename();
    bool useSSE = m_connectionDialog->useSSE();
    
    qInfo() << "[" << timestamp << "] MainWindow::onConnectionDialogAccepted - Connection accepted - URL:" << serverUrl.toString()
            << "QML file:" << qmlFilename << "Use SSE:" << useSSE;
    
    // Update network loader settings
    m_networkLoader->setUpdateInterval(m_connectionDialog->updateInterval());
    
    // Connect
    if (m_networkLoader->connectToServer(serverUrl, qmlFilename, useSSE)) {
        m_isLoading = true;
        m_loadingProgressBar->show();
        updateStatusBar();
    } else {
        qCritical() << "[" << timestamp << "] MainWindow::onConnectionDialogAccepted - Failed to initiate connection";
    }
}

void MainWindow::onConnectionDialogRejected()
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qInfo() << "[" << timestamp << "] MainWindow::onConnectionDialogRejected - Connection dialog cancelled";
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
    Q_UNUSED(content)
    // Legacy single-file loading — no longer used.
    // All loading now goes through loadQmlFromFolder().
}

void MainWindow::loadQmlFromFolder(const QString &qmlDir)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qInfo() << "[" << timestamp << "] MainWindow::loadQmlFromFolder - qmlDir:" << qmlDir;

    m_qmlDir = qmlDir;

    // Determine entry point (from settings, default "main.qml")
    QString entryPoint = m_settings->qmlFilename();
    if (entryPoint.isEmpty()) {
        entryPoint = "main.qml";
    }

    QString mainQmlPath = qmlDir + "/" + entryPoint;
    QUrl fileUrl = QUrl::fromLocalFile(mainQmlPath);

    qInfo() << "[" << timestamp << "] MainWindow::loadQmlFromFolder - Loading:" << fileUrl.toString();

    // Clear existing content
    m_quickWidget->setSource(QUrl());

    // Add the qml folder as an import path so "import components" works
    m_qmlEngine->addImportPath(qmlDir);

    // Clear component cache to force recompilation
    m_qmlEngine->clearComponentCache();

    m_quickWidget->setSource(fileUrl);

    // Check for errors
    if (m_quickWidget->status() == QQuickWidget::Error) {
        QStringList errors;
        for (const QQmlError &err : m_quickWidget->errors()) {
            qCritical() << "[" << timestamp << "] QML Error:" << err.url().toString()
                        << ":" << err.line() << "-" << err.description();
            errors << QString("%1:%2 - %3").arg(err.url().toString()).arg(err.line()).arg(err.description());
        }
        showError("QML Error:\n" + errors.join("\n"));
    }
}

void MainWindow::onBundleDownloaded(const QString &qmlDir)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qInfo() << "[" << timestamp << "] MainWindow::onBundleDownloaded - qmlDir:" << qmlDir;

    loadQmlFromFolder(qmlDir);

    m_isLoading = false;
    m_loadingProgressBar->hide();

    m_updateIndicatorVisible = true;
    QTimer::singleShot(500, this, [this]() {
        m_updateIndicatorVisible = false;
        updateStatusBar();
    });

    updateStatusBar();
}

void MainWindow::onFileUpdated(const QString &relativePath)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qInfo() << "[" << timestamp << "] MainWindow::onFileUpdated - file:" << relativePath;

    // Determine entry point
    QString entryPoint = m_settings->qmlFilename();
    if (entryPoint.isEmpty()) {
        entryPoint = "main.qml";
    }

    // Clear component cache so QML re-reads changed files from disk
    m_qmlEngine->clearComponentCache();

    if (relativePath == entryPoint) {
        // Main file changed — full reload
        qInfo() << "[" << timestamp << "] Main file updated, reloading";
        loadQmlFromFolder(m_qmlDir);
    } else {
        // Component file changed — reload to pick up changes
        qInfo() << "[" << timestamp << "] Component updated, reloading";
        loadQmlFromFolder(m_qmlDir);
    }

    m_updateIndicatorVisible = true;
    QTimer::singleShot(500, this, [this]() {
        m_updateIndicatorVisible = false;
        updateStatusBar();
    });

    updateStatusBar();
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
