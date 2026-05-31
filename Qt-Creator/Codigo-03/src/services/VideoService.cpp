#include "services/VideoService.h"
#include "services/ApiClient.h"
#include <QFileInfo>
#include <QJsonObject>
#include <QDebug>

VideoService::VideoService(QObject* parent)
    : AbstractApiService(parent)
{
    // Conectar señal de progreso de carga desde ApiClient
    connect(m_apiClient, &ApiClient::uploadProgress,
            this, [this](const QString& endpoint, int percent) {
                if (endpoint == EP_UPLOAD)
                    emit uploadProgressUpdated(percent);
            });
}

void VideoService::get(const QString& endpoint)
{
    doGet(endpoint);
}

void VideoService::post(const QString& endpoint, const QJsonObject& payload)
{
    doPost(endpoint, payload);
}

// ─── Operaciones públicas ─────────────────────────────────────────────────

void VideoService::uploadVideo(const QString& filePath)
{
    QFileInfo fi(filePath);
    if (!fi.exists() || !fi.isFile()) {
        emit uploadFailed("El archivo no existe: " + filePath);
        return;
    }

    m_currentFile    = filePath;
    m_pendingEndpoint = EP_UPLOAD;

    emit uploadStarted(fi.fileName());
    doPostMultipart(EP_UPLOAD, filePath);
}

void VideoService::startRtspSession(const QString& rtspUrl)
{
    m_pendingEndpoint = EP_RTSP_START;
    QJsonObject payload;
    payload["rtsp_url"] = rtspUrl;
    doPost(EP_RTSP_START, payload);
}

void VideoService::stopRtspSession(int sessionId)
{
    m_pendingEndpoint = EP_RTSP_STOP;
    QJsonObject payload;
    payload["session_id"] = sessionId;
    doPost(EP_RTSP_STOP, payload);
}

void VideoService::getSessionStatus(int sessionId)
{
    doGet(QString("%1/%2").arg(EP_SESSION_STATUS).arg(sessionId));
}

// ─── Manejo de respuestas ─────────────────────────────────────────────────

void VideoService::handleSuccess(const QString& endpoint,
                                  const QJsonDocument& response)
{
    QJsonObject obj = response.object();

    if (endpoint == EP_UPLOAD) {
        SessionModel session = parseSession(obj.value("session").toObject());
        emit uploadFinished(session);

    } else if (endpoint == EP_RTSP_START) {
        SessionModel session = parseSession(obj.value("session").toObject());
        emit rtspStarted(session);

    } else if (endpoint == EP_RTSP_STOP) {
        emit rtspStopped();

    } else {
        // Status del session
        SessionModel session = parseSession(obj);
        emit sessionStatusReceived(session);
    }
}

void VideoService::handleError(const QString& endpoint,
                                const QString& errorMessage,
                                int httpStatusCode)
{
    Q_UNUSED(httpStatusCode)

    if (endpoint == EP_UPLOAD)
        emit uploadFailed(errorMessage);
    else if (endpoint == EP_RTSP_START)
        emit rtspStartFailed(errorMessage);
    else if (endpoint == EP_RTSP_STOP)
        emit rtspStopFailed(errorMessage);
}

// ─── Helpers ──────────────────────────────────────────────────────────────

SessionModel VideoService::parseSession(const QJsonObject& obj) const
{
    QVariantMap map;
    map["id"]             = obj.value("id").toInt();
    map["user_id"]        = obj.value("user_id").toInt();
    map["source_type"]    = obj.value("source_type").toString();
    map["source_url"]     = obj.value("source_url").toString();
    map["filename"]       = obj.value("filename").toString();
    map["total_vehicles"] = obj.value("total_vehicles").toInt();
    map["started_at"]     = obj.value("started_at").toString();
    map["ended_at"]       = obj.value("ended_at").toString();
    map["upload_status"]  = obj.value("upload_status").toInt();
    return SessionModel::fromMap(map);
}
