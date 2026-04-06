#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <QSettings>
#include <QVariant>

/**
 * @brief Application settings manager for persistence
 * 
 * Handles saving and loading connection settings and application preferences
 * using QSettings for platform-specific storage.
 */
class Settings
{
public:
    /**
     * @brief Constructor - initializes settings with default values
     */
    Settings();
    
    /**
     * @brief Destructor
     */
    ~Settings();

    /**
     * @brief Load settings from persistent storage
     */
    void load();

    /**
     * @brief Save settings to persistent storage
     */
    void save();

    // Connection settings
    QString serverHost() const;
    void setServerHost(const QString &host);

    int serverPort() const;
    void setServerPort(int port);

    QString qmlFilename() const;
    void setQmlFilename(const QString &filename);

    // Application settings
    int updateInterval() const;
    void setUpdateInterval(int milliseconds);

    bool useSSE() const;
    void setUseSSE(bool enabled);

    bool autoReconnect() const;
    void setAutoReconnect(bool enabled);

    int reconnectDelay() const;
    void setReconnectDelay(int seconds);

    // Window settings
    QByteArray windowGeometry() const;
    void setWindowGeometry(const QByteArray &geometry);

    QByteArray windowState() const;
    void setWindowState(const QByteArray &state);

    // Connection history
    QStringList recentConnections() const;
    void addRecentConnection(const QString &connection);
    void clearRecentConnections();

    /**
     * @brief Reset all settings to defaults
     */
    void resetToDefaults();

private:
    QSettings *m_settings;
    
    // Connection settings
    QString m_serverHost;
    int m_serverPort;
    QString m_qmlFilename;
    
    // Application settings
    int m_updateInterval;
    bool m_useSSE;
    bool m_autoReconnect;
    int m_reconnectDelay;
    
    // Window settings
    QByteArray m_windowGeometry;
    QByteArray m_windowState;
    
    // Connection history
    QStringList m_recentConnections;
    
    static const int MAX_RECENT_CONNECTIONS = 10;
};

#endif // SETTINGS_H
