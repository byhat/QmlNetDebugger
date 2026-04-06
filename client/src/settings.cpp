#include "settings.h"

#include <QCoreApplication>

Settings::Settings()
    : m_settings(nullptr)
    , m_serverHost("localhost")
    , m_serverPort(8080)
    , m_qmlFilename("main.qml")
    , m_updateInterval(1000)  // 1 second
    , m_useSSE(true)
    , m_autoReconnect(true)
    , m_reconnectDelay(5)
{
    // Initialize QSettings with organization and application name
    m_settings = new QSettings("QmlNetDebugger", "QmlNetDebuggerClient");
    
    // Load settings on construction
    load();
}

Settings::~Settings()
{
    // Save settings before destruction
    save();
    
    if (m_settings) {
        delete m_settings;
        m_settings = nullptr;
    }
}

void Settings::load()
{
    // Load connection settings
    m_serverHost = m_settings->value("connection/serverHost", "localhost").toString();
    m_serverPort = m_settings->value("connection/serverPort", 8080).toInt();
    m_qmlFilename = m_settings->value("connection/qmlFilename", "main.qml").toString();
    
    // Load application settings
    m_updateInterval = m_settings->value("app/updateInterval", 1000).toInt();
    m_useSSE = m_settings->value("app/useSSE", true).toBool();
    m_autoReconnect = m_settings->value("app/autoReconnect", true).toBool();
    m_reconnectDelay = m_settings->value("app/reconnectDelay", 5).toInt();
    
    // Load window settings
    m_windowGeometry = m_settings->value("window/geometry").toByteArray();
    m_windowState = m_settings->value("window/state").toByteArray();
    
    // Load recent connections
    m_recentConnections = m_settings->value("history/recentConnections").toStringList();
}

void Settings::save()
{
    // Save connection settings
    m_settings->setValue("connection/serverHost", m_serverHost);
    m_settings->setValue("connection/serverPort", m_serverPort);
    m_settings->setValue("connection/qmlFilename", m_qmlFilename);
    
    // Save application settings
    m_settings->setValue("app/updateInterval", m_updateInterval);
    m_settings->setValue("app/useSSE", m_useSSE);
    m_settings->setValue("app/autoReconnect", m_autoReconnect);
    m_settings->setValue("app/reconnectDelay", m_reconnectDelay);
    
    // Save window settings
    if (!m_windowGeometry.isEmpty()) {
        m_settings->setValue("window/geometry", m_windowGeometry);
    }
    if (!m_windowState.isEmpty()) {
        m_settings->setValue("window/state", m_windowState);
    }
    
    // Save recent connections
    m_settings->setValue("history/recentConnections", m_recentConnections);
    
    m_settings->sync();
}

// Connection settings getters
QString Settings::serverHost() const
{
    return m_serverHost;
}

int Settings::serverPort() const
{
    return m_serverPort;
}

QString Settings::qmlFilename() const
{
    return m_qmlFilename;
}

// Connection settings setters
void Settings::setServerHost(const QString &host)
{
    m_serverHost = host;
}

void Settings::setServerPort(int port)
{
    m_serverPort = port;
}

void Settings::setQmlFilename(const QString &filename)
{
    m_qmlFilename = filename;
}

// Application settings getters
int Settings::updateInterval() const
{
    return m_updateInterval;
}

bool Settings::useSSE() const
{
    return m_useSSE;
}

bool Settings::autoReconnect() const
{
    return m_autoReconnect;
}

int Settings::reconnectDelay() const
{
    return m_reconnectDelay;
}

// Application settings setters
void Settings::setUpdateInterval(int milliseconds)
{
    m_updateInterval = milliseconds;
}

void Settings::setUseSSE(bool enabled)
{
    m_useSSE = enabled;
}

void Settings::setAutoReconnect(bool enabled)
{
    m_autoReconnect = enabled;
}

void Settings::setReconnectDelay(int seconds)
{
    m_reconnectDelay = seconds;
}

// Window settings getters
QByteArray Settings::windowGeometry() const
{
    return m_windowGeometry;
}

QByteArray Settings::windowState() const
{
    return m_windowState;
}

// Window settings setters
void Settings::setWindowGeometry(const QByteArray &geometry)
{
    m_windowGeometry = geometry;
}

void Settings::setWindowState(const QByteArray &state)
{
    m_windowState = state;
}

// Connection history
QStringList Settings::recentConnections() const
{
    return m_recentConnections;
}

void Settings::addRecentConnection(const QString &connection)
{
    // Remove if already exists
    m_recentConnections.removeAll(connection);
    
    // Add to front
    m_recentConnections.prepend(connection);
    
    // Trim to max size
    while (m_recentConnections.size() > MAX_RECENT_CONNECTIONS) {
        m_recentConnections.removeLast();
    }
}

void Settings::clearRecentConnections()
{
    m_recentConnections.clear();
}

void Settings::resetToDefaults()
{
    m_serverHost = "localhost";
    m_serverPort = 8080;
    m_qmlFilename = "main.qml";
    m_updateInterval = 1000;
    m_useSSE = true;
    m_autoReconnect = true;
    m_reconnectDelay = 5;
    m_windowGeometry.clear();
    m_windowState.clear();
    m_recentConnections.clear();
    
    save();
}
