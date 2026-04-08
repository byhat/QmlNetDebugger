#include "connectiondialog.h"

#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QDebug>
#include <QDateTime>
#include <QPainter>
#include <QResizeEvent>
#include <QKeyEvent>

ConnectionDialog::ConnectionDialog(Settings *settings, QWidget *parent)
    : QWidget(parent)
    , m_panel(nullptr)
    , m_hostEdit(nullptr)
    , m_portSpinBox(nullptr)
    , m_filenameEdit(nullptr)
    , m_recentCombo(nullptr)
    , m_useSSECheckBox(nullptr)
    , m_updateIntervalSpinBox(nullptr)
    , m_connectButton(nullptr)
    , m_cancelButton(nullptr)
    , m_settings(settings)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qInfo() << "[" << timestamp << "] ConnectionDialog::ConnectionDialog - Initializing inline connection dialog";

    setupUi();
    setupConnections();
    loadSettings();
    loadRecentConnections();

    // Install event filter on the panel to intercept Escape key
    m_panel->installEventFilter(this);

    // Initially hidden
    hide();

    qInfo() << "[" << timestamp << "] ConnectionDialog::ConnectionDialog - Inline connection dialog initialized";
}

ConnectionDialog::~ConnectionDialog()
{
    // Settings are saved by the caller
}

void ConnectionDialog::setupUi()
{
    // Make this widget fill the parent and act as a semi-transparent overlay
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setAttribute(Qt::WA_StyledBackground, true);

    // Main layout for the overlay - centers the panel
    auto *overlayLayout = new QVBoxLayout(this);
    overlayLayout->setContentsMargins(0, 0, 0, 0);
    overlayLayout->setAlignment(Qt::AlignCenter);

    // Create the panel (the actual dialog content)
    m_panel = new QFrame(this);
    m_panel->setFrameShape(QFrame::StyledPanel);
    m_panel->setFrameShadow(QFrame::Raised);
    m_panel->setLineWidth(1);
    m_panel->setMidLineWidth(0);
    m_panel->setMinimumWidth(420);
    m_panel->setMaximumWidth(500);
    m_panel->setStyleSheet(
        "QFrame {"
        "   background-color: #ffffff;"
        "   border: 1px solid #cccccc;"
        "   border-radius: 8px;"
        "}"
    );

    auto *panelLayout = new QVBoxLayout(m_panel);
    panelLayout->setContentsMargins(20, 20, 20, 20);
    panelLayout->setSpacing(12);

    // Title label
    auto *titleLabel = new QLabel("Connect to Server", m_panel);
    QFont titleFont;
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    panelLayout->addWidget(titleLabel);

    // Separator
    auto *separator = new QFrame(m_panel);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    panelLayout->addWidget(separator);

    // Connection settings group
    auto *connectionGroup = new QGroupBox("Connection Settings", m_panel);
    auto *connectionLayout = new QFormLayout(connectionGroup);

    // Server host
    m_hostEdit = new QLineEdit(m_panel);
    m_hostEdit->setPlaceholderText("e.g., localhost or 192.168.1.100");
    connectionLayout->addRow("Server Host:", m_hostEdit);

    // Server port
    m_portSpinBox = new QSpinBox(m_panel);
    m_portSpinBox->setRange(1, 65535);
    m_portSpinBox->setValue(8080);
    connectionLayout->addRow("Server Port:", m_portSpinBox);

    // QML entry point
    m_filenameEdit = new QLineEdit(m_panel);
    m_filenameEdit->setPlaceholderText("e.g., main.qml");
    connectionLayout->addRow("Entry Point QML:", m_filenameEdit);

    panelLayout->addWidget(connectionGroup);

    // Recent connections group
    auto *recentGroup = new QGroupBox("Recent Connections", m_panel);
    auto *recentLayout = new QVBoxLayout(recentGroup);

    m_recentCombo = new QComboBox(m_panel);
    m_recentCombo->setEditable(false);
    recentLayout->addWidget(m_recentCombo);

    panelLayout->addWidget(recentGroup);

    // Advanced settings group
    auto *advancedGroup = new QGroupBox("Advanced Settings", m_panel);
    auto *advancedLayout = new QFormLayout(advancedGroup);

    // Use SSE checkbox
    m_useSSECheckBox = new QCheckBox("Use Server-Sent Events", m_panel);
    m_useSSECheckBox->setChecked(true);
    m_useSSECheckBox->setToolTip("Enable real-time updates via SSE (recommended)");
    advancedLayout->addRow(m_useSSECheckBox);

    // Update interval
    m_updateIntervalSpinBox = new QSpinBox(m_panel);
    m_updateIntervalSpinBox->setRange(100, 60000);
    m_updateIntervalSpinBox->setSuffix(" ms");
    m_updateIntervalSpinBox->setValue(1000);
    m_updateIntervalSpinBox->setToolTip("Polling interval when SSE is disabled");
    advancedLayout->addRow("Update Interval:", m_updateIntervalSpinBox);

    panelLayout->addWidget(advancedGroup);

    // Button layout
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_connectButton = new QPushButton("Connect", m_panel);
    m_connectButton->setDefault(true);
    m_connectButton->setMinimumWidth(100);
    buttonLayout->addWidget(m_connectButton);

    m_cancelButton = new QPushButton("Cancel", m_panel);
    m_cancelButton->setMinimumWidth(100);
    buttonLayout->addWidget(m_cancelButton);

    panelLayout->addLayout(buttonLayout);

    overlayLayout->addWidget(m_panel);
    setLayout(overlayLayout);
}

void ConnectionDialog::setupConnections()
{
    connect(m_connectButton, &QPushButton::clicked, this, &ConnectionDialog::onConnectClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &ConnectionDialog::onCancelClicked);
    connect(m_recentCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConnectionDialog::onRecentConnectionSelected);
}

void ConnectionDialog::loadSettings()
{
    m_hostEdit->setText(m_settings->serverHost());
    m_portSpinBox->setValue(m_settings->serverPort());
    m_filenameEdit->setText(m_settings->qmlFilename());
    m_useSSECheckBox->setChecked(m_settings->useSSE());
    m_updateIntervalSpinBox->setValue(m_settings->updateInterval());
}

void ConnectionDialog::saveSettings()
{
    m_settings->setServerHost(m_hostEdit->text().trimmed());
    m_settings->setServerPort(m_portSpinBox->value());
    m_settings->setQmlFilename(m_filenameEdit->text().trimmed());
    m_settings->setUseSSE(m_useSSECheckBox->isChecked());
    m_settings->setUpdateInterval(m_updateIntervalSpinBox->value());

    // Add to recent connections
    QString connection = QString("%1:%2|%3")
                            .arg(m_hostEdit->text().trimmed())
                            .arg(m_portSpinBox->value())
                            .arg(m_filenameEdit->text().trimmed());
    m_settings->addRecentConnection(connection);

    m_settings->save();
}

void ConnectionDialog::loadRecentConnections()
{
    m_recentCombo->clear();
    m_recentCombo->addItem("-- Select Recent Connection --");

    QStringList recent = m_settings->recentConnections();
    for (const QString &connection : recent) {
        m_recentCombo->addItem(connection);
    }
}

void ConnectionDialog::onConnectClicked()
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qInfo() << "[" << timestamp << "] ConnectionDialog::onConnectClicked - Connect button clicked";

    if (validateInput()) {
        saveSettings();
        qInfo() << "[" << timestamp << "] ConnectionDialog::onConnectClicked - Validation passed, accepting dialog";
        hide();
        emit accepted();
    } else {
        qWarning() << "[" << timestamp << "] ConnectionDialog::onConnectClicked - Validation failed";
    }
}

void ConnectionDialog::onCancelClicked()
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qInfo() << "[" << timestamp << "] ConnectionDialog::onCancelClicked - Cancel button clicked, rejecting dialog";

    hide();
    emit rejected();
}

void ConnectionDialog::onRecentConnectionSelected(int index)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");

    if (index <= 0) {
        qInfo() << "[" << timestamp << "] ConnectionDialog::onRecentConnectionSelected - Default item selected, ignoring";
        return; // Default item selected
    }

    QString connection = m_recentCombo->currentText();
    qInfo() << "[" << timestamp << "] ConnectionDialog::onRecentConnectionSelected - Selected recent connection:" << connection;

    // Parse connection string: "host:port|filename"
    QStringList parts = connection.split('|');
    if (parts.size() == 2) {
        QStringList hostPort = parts[0].split(':');
        if (hostPort.size() == 2) {
            m_hostEdit->setText(hostPort[0]);
            m_portSpinBox->setValue(hostPort[1].toInt());
            m_filenameEdit->setText(parts[1]);
            qInfo() << "[" << timestamp << "] ConnectionDialog::onRecentConnectionSelected - Loaded connection - Host:"
                    << hostPort[0] << "Port:" << hostPort[1] << "File:" << parts[1];
        } else {
            qWarning() << "[" << timestamp << "] ConnectionDialog::onRecentConnectionSelected - Failed to parse host:port from:" << parts[0];
        }
    } else {
        qWarning() << "[" << timestamp << "] ConnectionDialog::onRecentConnectionSelected - Failed to parse connection string:" << connection;
    }

    // Reset combo box to default
    m_recentCombo->setCurrentIndex(0);
}

bool ConnectionDialog::validateInput()
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString host = m_hostEdit->text().trimmed();
    QString filename = m_filenameEdit->text().trimmed();

    qInfo() << "[" << timestamp << "] ConnectionDialog::validateInput - Validating input - Host:" << host
            << "Port:" << m_portSpinBox->value() << "Filename:" << filename;

    // Validate host
    if (host.isEmpty()) {
        qWarning() << "[" << timestamp << "] ConnectionDialog::validateInput - Validation failed: Host is empty";
        QMessageBox::warning(m_panel, "Validation Error", "Please enter a server host.");
        m_hostEdit->setFocus();
        return false;
    }

    // Validate port
    if (m_portSpinBox->value() < 1 || m_portSpinBox->value() > 65535) {
        qWarning() << "[" << timestamp << "] ConnectionDialog::validateInput - Validation failed: Invalid port:" << m_portSpinBox->value();
        QMessageBox::warning(m_panel, "Validation Error", "Please enter a valid port number (1-65535).");
        m_portSpinBox->setFocus();
        return false;
    }

    // Validate filename
    if (filename.isEmpty()) {
        qWarning() << "[" << timestamp << "] ConnectionDialog::validateInput - Validation failed: Filename is empty";
        QMessageBox::warning(m_panel, "Validation Error", "Please enter a QML filename.");
        m_filenameEdit->setFocus();
        return false;
    }

    // Check if filename has .qml extension
    if (!filename.endsWith(".qml", Qt::CaseInsensitive)) {
        qWarning() << "[" << timestamp << "] ConnectionDialog::validateInput - Validation failed: Filename missing .qml extension:" << filename;
        QMessageBox::warning(m_panel, "Validation Error",
                           "QML filename should have a .qml extension.");
        m_filenameEdit->setFocus();
        return false;
    }

    qInfo() << "[" << timestamp << "] ConnectionDialog::validateInput - Validation passed";
    return true;
}

QUrl ConnectionDialog::serverUrl() const
{
    QUrl url;
    url.setScheme("http");
    url.setHost(m_hostEdit->text().trimmed());
    url.setPort(m_portSpinBox->value());
    return url;
}

QString ConnectionDialog::qmlFilename() const
{
    return m_filenameEdit->text().trimmed();
}

int ConnectionDialog::updateInterval() const
{
    return m_updateIntervalSpinBox->value();
}

bool ConnectionDialog::useSSE() const
{
    return m_useSSECheckBox->isChecked();
}

void ConnectionDialog::showDialog()
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qInfo() << "[" << timestamp << "] ConnectionDialog::showDialog - Showing inline connection dialog";

    // Reload settings in case they changed
    loadSettings();
    loadRecentConnections();

    // Resize to fill parent
    if (parentWidget()) {
        setGeometry(parentWidget()->rect());
    }

    raise();
    show();
    setFocus();

    // Focus the host edit field
    m_hostEdit->setFocus();
    m_hostEdit->selectAll();
}

void ConnectionDialog::hideDialog()
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qInfo() << "[" << timestamp << "] ConnectionDialog::hideDialog - Hiding inline connection dialog";
    hide();
}

void ConnectionDialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    // Draw semi-transparent overlay background
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, 128));
}

void ConnectionDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        onCancelClicked();
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

bool ConnectionDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_panel && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            onCancelClicked();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}
