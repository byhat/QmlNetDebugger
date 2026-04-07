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
#include <QFile>
#include <QDir>

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
    void connectionStateChanged(ConnectionState state);
    void qmlLoaded(const QString &content, const QString &etag);
    void qmlUnchanged();
    void errorOccurred(const QString &error);
    void updateCheckCompleted(bool updated);

    /**
     * @brief Emitted after the full QML bundle is downloaded and written to disk
     * @param qmlDir Local path to the qml folder (e.g. "./tmp/qml")
     */
    void bundleDownloaded(const QString &qmlDir);

    /**
     * @brief Emitted after a single file is updated via SSE
     * @param relativePath Relative path of the updated file
     */
    void fileUpdated(const QString &relativePath);

private slots:
    void onQmlDownloadFinished();
    void onBundleDownloadFinished();
    void onSingleFileDownloadFinished();
    void onSseDataReceived();
    void onSseError();
    void onPollingTimeout();
    void onNetworkError(QNetworkReply *reply);

private:
    void downloadQmlFile(bool useETag = true);
    void downloadBundle();
    void downloadSingleFile(const QString &relativePath);
    void startSseConnection();
    void stopSseConnection();
    void startPolling();
    void stopPolling();
    void parseSseEvent(const QString &data);
    void setConnectionState(ConnectionState state);
    void handleHttpError(int statusCode, const QString &reason);
    bool ensureDirectoryExists(const QString &filePath);

    QNetworkAccessManager *m_networkManager;
    QUrl m_serverUrl;
    QString m_qmlFilename;
    bool m_useSSE;

    ConnectionState m_connectionState;
    QString m_qmlContent;
    QString m_currentETag;
    QDateTime m_lastUpdateTime;
    QString m_lastError;

    QTimer *m_pollingTimer;
    int m_updateInterval;
    QNetworkReply *m_sseReply;
    QByteArray m_sseBuffer;

    int m_retryCount;
    static const int MAX_RETRIES = 3;

    int m_sseRetryCount;
    static const int MAX_SSE_RETRIES = 10;

    QString m_qmlLocalDir;  // "./tmp/qml" - local storage path
    QString m_pendingSingleFilePath; // tracks which file a single-file download is for
};

#endif // QMLNETWORKLOADER_H
