#ifndef QMLNETWORKLOADER_H
#define QMLNETWORKLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QString>
#include <QUrl>
#include <QDateTime>
#include <QMap>
#include <QMutex>

/**
 * @brief Network loader for QML files from remote server
 * 
 * Handles HTTP requests to download QML files, supports ETag-based
 * conditional requests, and implements both polling and SSE update mechanisms.
 */
class QmlNetworkLoader : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Connection state enumeration
     */
    enum class ConnectionState {
        Disconnected,
        Connecting,
        Connected,
        Error
    };
    Q_ENUM(ConnectionState)

    /**
     * @brief Constructor
     * @param parent Parent object
     */
    explicit QmlNetworkLoader(QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~QmlNetworkLoader() override;

    /**
     * @brief Connect to the server
     * @param serverUrl Server URL
     * @param qmlFilename QML file to load
     * @param useSSE Whether to use SSE for updates
     * @return True if connection initiated successfully
     */
    bool connectToServer(const QUrl &serverUrl, const QString &qmlFilename, bool useSSE);

    /**
     * @brief Disconnect from the server
     */
    void disconnectFromServer();

    /**
     * @brief Manually refresh the QML file
     */
    void refresh();

    /**
     * @brief Set the update interval for polling
     * @param milliseconds Update interval in milliseconds
     */
    void setUpdateInterval(int milliseconds);

    /**
     * @brief Get the current connection state
     * @return Connection state
     */
    ConnectionState connectionState() const;

    /**
     * @brief Get the last update timestamp
     * @return Last update time
     */
    QDateTime lastUpdateTime() const;

    /**
     * @brief Get the last error message
     * @return Error message
     */
    QString lastError() const;

    /**
     * @brief Get the current QML content
     * @return QML content
     */
    QString qmlContent() const;

    /**
     * @brief Get the current ETag
     * @return ETag string
     */
    QString currentETag() const;

signals:
    /**
     * @brief Emitted when connection state changes
     * @param state New connection state
     */
    void connectionStateChanged(ConnectionState state);

    /**
     * @brief Emitted when QML content is loaded or updated
     * @param content QML content
     * @param etag ETag of the content
     */
    void qmlLoaded(const QString &content, const QString &etag);

    /**
     * @brief Emitted when QML content is unchanged (304 Not Modified)
     */
    void qmlUnchanged();

    /**
     * @brief Emitted when an error occurs
     * @param error Error message
     */
    void errorOccurred(const QString &error);

    /**
     * @brief Emitted when update check completes
     * @param updated True if content was updated
     */
    void updateCheckCompleted(bool updated);

private slots:
    /**
     * @brief Handle QML file download completion
     */
    void onQmlDownloadFinished();

    /**
     * @brief Handle SSE data received
     */
    void onSseDataReceived();

    /**
     * @brief Handle SSE connection error
     */
    void onSseError();

    /**
     * @brief Handle polling timer timeout
     */
    void onPollingTimeout();

    /**
     * @brief Handle network error
     * @param reply Network reply that failed
     */
    void onNetworkError(QNetworkReply *reply);

private:
    /**
     * @brief Download QML file from server
     * @param useETag Whether to use ETag for conditional request
     */
    void downloadQmlFile(bool useETag = true);

    /**
     * @brief Start SSE connection
     */
    void startSseConnection();

    /**
     * @brief Stop SSE connection
     */
    void stopSseConnection();

    /**
     * @brief Start polling
     */
    void startPolling();

    /**
     * @brief Stop polling
     */
    void stopPolling();

    /**
     * @brief Parse SSE event
     * @param data SSE event data
     */
    void parseSseEvent(const QString &data);

    /**
     * @brief Set connection state
     * @param state New connection state
     */
    void setConnectionState(ConnectionState state);

    /**
     * @brief Handle HTTP error response
     * @param statusCode HTTP status code
     * @param reason Error reason
     */
    void handleHttpError(int statusCode, const QString &reason);

    // Network manager
    QNetworkAccessManager *m_networkManager;

    // Connection parameters
    QUrl m_serverUrl;
    QString m_qmlFilename;
    bool m_useSSE;

    // Current state
    ConnectionState m_connectionState;
    QString m_qmlContent;
    QString m_currentETag;
    QDateTime m_lastUpdateTime;
    QString m_lastError;

    // Update mechanism
    QTimer *m_pollingTimer;
    int m_updateInterval;
    QNetworkReply *m_sseReply;
    QByteArray m_sseBuffer;

    // Retry logic
    int m_retryCount;
    static const int MAX_RETRIES = 3;
};

#endif // QMLNETWORKLOADER_H
