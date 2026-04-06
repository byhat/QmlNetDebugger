#include "qmlnetworkloader.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrlQuery>
#include <QDebug>
#include <QDateTime>

QmlNetworkLoader::QmlNetworkLoader(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_useSSE(false)
    , m_connectionState(ConnectionState::Disconnected)
    , m_updateInterval(1000)
    , m_pollingTimer(new QTimer(this))
    , m_sseReply(nullptr)
    , m_retryCount(0)
{
    // Setup polling timer
    m_pollingTimer->setSingleShot(false);
    connect(m_pollingTimer, &QTimer::timeout, this, &QmlNetworkLoader::onPollingTimeout);
}

QmlNetworkLoader::~QmlNetworkLoader()
{
    disconnectFromServer();
}

bool QmlNetworkLoader::connectToServer(const QUrl &serverUrl, const QString &qmlFilename, bool useSSE)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qInfo() << "[" << timestamp << "] QmlNetworkLoader::connectToServer - URL:" << serverUrl.toString()
            << "QML file:" << qmlFilename << "Use SSE:" << useSSE;
    
    // Validate URL
    if (!serverUrl.isValid()) {
        m_lastError = "Invalid server URL";
        QString errorTimestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        qCritical() << "[" << errorTimestamp << "] QmlNetworkLoader::connectToServer - ERROR: Invalid server URL:" << serverUrl.toString();
        emit errorOccurred(m_lastError);
        return false;
    }

    // Store connection parameters
    m_serverUrl = serverUrl;
    m_qmlFilename = qmlFilename;
    m_useSSE = useSSE;
    m_retryCount = 0;

    // Set connecting state
    setConnectionState(ConnectionState::Connecting);

    // Initial download
    downloadQmlFile(false);

    return true;
}

void QmlNetworkLoader::disconnectFromServer()
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qInfo() << "[" << timestamp << "] QmlNetworkLoader::disconnectFromServer - Disconnecting from server";
    
    // Stop polling
    stopPolling();

    // Stop SSE
    stopSseConnection();

    // Reset state
    setConnectionState(ConnectionState::Disconnected);
    m_qmlContent.clear();
    m_currentETag.clear();
    m_lastUpdateTime = QDateTime();
    m_lastError.clear();
    
    qInfo() << "[" << timestamp << "] QmlNetworkLoader::disconnectFromServer - Disconnected successfully";
}

void QmlNetworkLoader::refresh()
{
    if (m_connectionState != ConnectionState::Connected) {
        m_lastError = "Not connected to server";
        emit errorOccurred(m_lastError);
        return;
    }

    downloadQmlFile(true);
}

void QmlNetworkLoader::setUpdateInterval(int milliseconds)
{
    m_updateInterval = milliseconds;
    
    if (m_pollingTimer->isActive()) {
        m_pollingTimer->setInterval(m_updateInterval);
    }
}

QmlNetworkLoader::ConnectionState QmlNetworkLoader::connectionState() const
{
    return m_connectionState;
}

QDateTime QmlNetworkLoader::lastUpdateTime() const
{
    return m_lastUpdateTime;
}

QString QmlNetworkLoader::lastError() const
{
    return m_lastError;
}

QString QmlNetworkLoader::qmlContent() const
{
    return m_qmlContent;
}

QString QmlNetworkLoader::currentETag() const
{
    return m_currentETag;
}

void QmlNetworkLoader::downloadQmlFile(bool useETag)
{
    // Build file URL
    QUrl fileUrl = m_serverUrl;
    QString path = QString("/api/file/%1").arg(m_qmlFilename);
    fileUrl.setPath(path);

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qInfo() << "[" << timestamp << "] QmlNetworkLoader::downloadQmlFile - Requesting:" << fileUrl.toString()
            << "Use ETag:" << useETag << "Current ETag:" << (useETag ? m_currentETag : "(none)");

    // Create request
    QNetworkRequest request(fileUrl);
    request.setRawHeader("Accept", "application/x-qml");

    // Add ETag header if available
    if (useETag && !m_currentETag.isEmpty()) {
        request.setRawHeader("If-None-Match", m_currentETag.toUtf8());
        qInfo() << "[" << timestamp << "] QmlNetworkLoader::downloadQmlFile - Added If-None-Match header:" << m_currentETag;
    }

    // Send request
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &QmlNetworkLoader::onQmlDownloadFinished);
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
            this, [this, reply](QNetworkReply::NetworkError error) {
                onNetworkError(reply);
            });
}

void QmlNetworkLoader::onQmlDownloadFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        qCritical() << "[" << timestamp << "] QmlNetworkLoader::onQmlDownloadFinished - ERROR: Reply is null";
        return;
    }

    reply->deleteLater();

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qInfo() << "[" << timestamp << "] QmlNetworkLoader::onQmlDownloadFinished - Status code:" << statusCode;

    // Handle different status codes
    switch (statusCode) {
        case 200: {
            // Success - new content
            QByteArray content = reply->readAll();
            m_qmlContent = QString::fromUtf8(content);
            
            // Get ETag
            QString newETag = QString::fromUtf8(reply->rawHeader("ETag"));
            if (!newETag.isEmpty()) {
                m_currentETag = newETag;
            }
            
            m_lastUpdateTime = QDateTime::currentDateTime();
            m_lastError.clear();
            m_retryCount = 0;
            
            QString successTimestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
            qInfo() << "[" << successTimestamp << "] QmlNetworkLoader::onQmlDownloadFinished - SUCCESS: Loaded QML content"
                    << "Size:" << content.size() << "bytes" << "ETag:" << m_currentETag;
            
            // Update state if this was initial connection
            if (m_connectionState == ConnectionState::Connecting) {
                setConnectionState(ConnectionState::Connected);
                
                // Start update mechanism
                if (m_useSSE) {
                    startSseConnection();
                } else {
                    startPolling();
                }
            }
            
            emit qmlLoaded(m_qmlContent, m_currentETag);
            emit updateCheckCompleted(true);
            break;
        }
        
        case 304: {
            // Not Modified - content unchanged
            m_lastUpdateTime = QDateTime::currentDateTime();
            m_lastError.clear();
            
            qInfo() << "[" << timestamp << "] QmlNetworkLoader::onQmlDownloadFinished - Content unchanged (304 Not Modified)";
            
            emit qmlUnchanged();
            emit updateCheckCompleted(false);
            break;
        }
        
        case 404: {
            // File not found
            QString error = QString("QML file not found: %1").arg(m_qmlFilename);
            m_lastError = error;
            handleHttpError(statusCode, error);
            break;
        }
        
        case 403: {
            // Forbidden
            QString error = "Access denied to QML file";
            m_lastError = error;
            handleHttpError(statusCode, error);
            break;
        }
        
        case 500: {
            // Server error
            QString error = "Server error occurred";
            m_lastError = error;
            handleHttpError(statusCode, error);
            break;
        }
        
        default: {
            // Other errors
            QString error = QString("HTTP error: %1").arg(statusCode);
            m_lastError = error;
            handleHttpError(statusCode, error);
            break;
        }
    }
}

void QmlNetworkLoader::startSseConnection()
{
    if (m_sseReply) {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        qWarning() << "[" << timestamp << "] QmlNetworkLoader::startSseConnection - SSE connection already exists, skipping";
        return;
    }

    // Build SSE URL
    QUrl sseUrl = m_serverUrl;
    sseUrl.setPath("/api/events");

    // Add query parameter for path filter
    QUrlQuery query;
    query.addQueryItem("path", m_qmlFilename);
    sseUrl.setQuery(query);

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qInfo() << "[" << timestamp << "] QmlNetworkLoader::startSseConnection - Connecting to SSE endpoint:" << sseUrl.toString();

    // Create request
    QNetworkRequest request(sseUrl);
    request.setRawHeader("Accept", "text/event-stream");
    request.setRawHeader("Cache-Control", "no-cache");

    // Send request
    m_sseReply = m_networkManager->get(request);
    connect(m_sseReply, &QNetworkReply::readyRead, this, &QmlNetworkLoader::onSseDataReceived);
    connect(m_sseReply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
            this, [this](QNetworkReply::NetworkError) {
                onSseError();
            });
    connect(m_sseReply, &QNetworkReply::finished, this, &QmlNetworkLoader::onSseError);
    
    qInfo() << "[" << timestamp << "] QmlNetworkLoader::startSseConnection - SSE connection initiated";
}

void QmlNetworkLoader::stopSseConnection()
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qInfo() << "[" << timestamp << "] QmlNetworkLoader::stopSseConnection - Stopping SSE connection";
    
    if (m_sseReply) {
        m_sseReply->disconnect();
        m_sseReply->abort();
        m_sseReply->deleteLater();
        m_sseReply = nullptr;
    }
    m_sseBuffer.clear();
    
    qInfo() << "[" << timestamp << "] QmlNetworkLoader::stopSseConnection - SSE connection stopped";
}

void QmlNetworkLoader::onSseDataReceived()
{
    if (!m_sseReply) {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        qCritical() << "[" << timestamp << "] QmlNetworkLoader::onSseDataReceived - ERROR: SSE reply is null";
        return;
    }

    // Append new data to buffer
    m_sseBuffer.append(m_sseReply->readAll());

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qDebug() << "[" << timestamp << "] QmlNetworkLoader::onSseDataReceived - Received SSE data, buffer size:" << m_sseBuffer.size();

    // Process complete events
    while (true) {
        int newlinePos = m_sseBuffer.indexOf("\n\n");
        if (newlinePos == -1) {
            break; // No complete event yet
        }

        // Extract event data
        QByteArray eventData = m_sseBuffer.left(newlinePos);
        m_sseBuffer = m_sseBuffer.mid(newlinePos + 2);

        // Parse event
        parseSseEvent(QString::fromUtf8(eventData));
    }
}

void QmlNetworkLoader::onSseError()
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qWarning() << "[" << timestamp << "] QmlNetworkLoader::onSseError - SSE connection error occurred";
    
    stopSseConnection();
    
    // Fall back to polling if SSE fails
    if (m_connectionState == ConnectionState::Connected) {
        m_lastError = "SSE connection lost, falling back to polling";
        qWarning() << "[" << timestamp << "] QmlNetworkLoader::onSseError - Falling back to polling";
        emit errorOccurred(m_lastError);
        startPolling();
    }
}

void QmlNetworkLoader::parseSseEvent(const QString &data)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qInfo() << "[" << timestamp << "] QmlNetworkLoader::parseSseEvent - Parsing SSE event:" << data;
    
    // Parse SSE event format: "event: <type>\ndata: <json>"
    QStringList lines = data.split('\n');
    
    QString eventType;
    QString jsonData;
    
    for (const QString &line : lines) {
        if (line.startsWith("event:")) {
            eventType = line.mid(6).trimmed();
        } else if (line.startsWith("data:")) {
            jsonData = line.mid(5).trimmed();
        }
    }
    
    qInfo() << "[" << timestamp << "] QmlNetworkLoader::parseSseEvent - Event type:" << eventType;
    
    // Handle file changed event
    if (eventType == "file_changed" && !jsonData.isEmpty()) {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8(), &parseError);
        
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();
            QString path = obj["path"].toString();
            QString action = obj["action"].toString();
            QString etag = obj["etag"].toString();
            
            qInfo() << "[" << timestamp << "] QmlNetworkLoader::parseSseEvent - File event - path:" << path
                    << "action:" << action << "etag:" << etag;
            
            // Check if this is our file
            if (path == m_qmlFilename && action == "modified") {
                qInfo() << "[" << timestamp << "] QmlNetworkLoader::parseSseEvent - File modified, downloading updated content";
                // Download updated content
                downloadQmlFile(false);
            }
        } else {
            qWarning() << "[" << timestamp << "] QmlNetworkLoader::parseSseEvent - Failed to parse JSON:" << parseError.errorString();
        }
    }
}

void QmlNetworkLoader::startPolling()
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qInfo() << "[" << timestamp << "] QmlNetworkLoader::startPolling - Starting polling with interval:" << m_updateInterval << "ms";
    
    stopSseConnection();
    m_pollingTimer->start(m_updateInterval);
}

void QmlNetworkLoader::stopPolling()
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qInfo() << "[" << timestamp << "] QmlNetworkLoader::stopPolling - Stopping polling";
    
    m_pollingTimer->stop();
}

void QmlNetworkLoader::onPollingTimeout()
{
    if (m_connectionState == ConnectionState::Connected) {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        qInfo() << "[" << timestamp << "] QmlNetworkLoader::onPollingTimeout - Polling timeout, checking for updates";
        downloadQmlFile(true);
    }
}

void QmlNetworkLoader::onNetworkError(QNetworkReply *reply)
{
    QString errorString = reply->errorString();
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qCritical() << "[" << timestamp << "] QmlNetworkLoader::onNetworkError - Network error:" << errorString
                << "Retry count:" << m_retryCount << "/" << MAX_RETRIES;
    
    m_lastError = QString("Network error: %1").arg(errorString);
    
    emit errorOccurred(m_lastError);
    
    // Handle retry logic
    if (m_connectionState == ConnectionState::Connecting && m_retryCount < MAX_RETRIES) {
        m_retryCount++;
        qInfo() << "[" << timestamp << "] QmlNetworkLoader::onNetworkError - Retrying in" << (1000 * m_retryCount) << "ms";
        QTimer::singleShot(1000 * m_retryCount, this, [this]() {
            downloadQmlFile(false);
        });
    } else {
        qCritical() << "[" << timestamp << "] QmlNetworkLoader::onNetworkError - Max retries reached or not in connecting state, setting error state";
        setConnectionState(ConnectionState::Error);
    }
    
    reply->deleteLater();
}

void QmlNetworkLoader::setConnectionState(ConnectionState state)
{
    if (m_connectionState != state) {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        QString stateStr;
        switch (state) {
            case ConnectionState::Disconnected: stateStr = "Disconnected"; break;
            case ConnectionState::Connecting: stateStr = "Connecting"; break;
            case ConnectionState::Connected: stateStr = "Connected"; break;
            case ConnectionState::Error: stateStr = "Error"; break;
        }
        qInfo() << "[" << timestamp << "] QmlNetworkLoader::setConnectionState - State changed:" << stateStr;
        
        m_connectionState = state;
        emit connectionStateChanged(state);
    }
}

void QmlNetworkLoader::handleHttpError(int statusCode, const QString &reason)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qCritical() << "[" << timestamp << "] QmlNetworkLoader::handleHttpError - HTTP error:" << statusCode << "Reason:" << reason;
    
    setConnectionState(ConnectionState::Error);
    emit errorOccurred(reason);
    emit updateCheckCompleted(false);
}
