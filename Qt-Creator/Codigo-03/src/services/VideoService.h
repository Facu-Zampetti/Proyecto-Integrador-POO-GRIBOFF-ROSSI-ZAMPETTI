#ifndef VIDEOSERVICE_H
#define VIDEOSERVICE_H

#include "base/AbstractApiService.h"
#include "models/SessionModel.h"
#include "models/VehicleModel.h"
#include <QList>

/**
 * @file VideoService.h
 * @brief Servicio de carga de videos y conexión RTSP. Hereda de AbstractApiService.
 *
 * Gestiona:
 *  - Subida de archivos MP4 al backend (multipart/form-data).
 *  - Inicio y detención de sesiones RTSP.
 *  - Consulta del estado de procesamiento.
 */
class VideoService : public AbstractApiService {
    Q_OBJECT

public:
    explicit VideoService(QObject* parent = nullptr);

    // ── Operaciones públicas ───────────────────────────────────────────
    void uploadVideo(const QString& filePath);
    void startRtspSession(const QString& rtspUrl);
    void stopRtspSession(int sessionId);
    void getSessionStatus(int sessionId);

    // ── Endpoints ─────────────────────────────────────────────────────
    static constexpr const char* EP_UPLOAD      = "/api/videos/upload";
    static constexpr const char* EP_RTSP_START  = "/api/rtsp/start";
    static constexpr const char* EP_RTSP_STOP   = "/api/rtsp/stop";
    static constexpr const char* EP_SESSION_STATUS = "/api/sessions/status";

    void get(const QString& endpoint) override;
    void post(const QString& endpoint, const QJsonObject& payload) override;

signals:
    void uploadStarted(const QString& filename);
    void uploadProgressUpdated(int percent);
    void uploadFinished(const SessionModel& session);
    void uploadFailed(const QString& reason);

    void rtspStarted(const SessionModel& session);
    void rtspStartFailed(const QString& reason);
    void rtspStopped();
    void rtspStopFailed(const QString& reason);

    void sessionStatusReceived(const SessionModel& session);

protected:
    void handleSuccess(const QString& endpoint,
                       const QJsonDocument& response) override;

    void handleError(const QString& endpoint,
                     const QString& errorMessage,
                     int httpStatusCode) override;

private:
    SessionModel parseSession(const QJsonObject& obj) const;

    QString m_currentFile;
    QString m_pendingEndpoint;
};

#endif // VIDEOSERVICE_H
