#ifndef RTSPMANAGER_H
#define RTSPMANAGER_H

#include <QObject>
#include <QTimer>
#include <QRegularExpression>
#include "base/IManager.h"
#include "models/SessionModel.h"
#include "services/VideoService.h"
#include "utils/AppEnums.h"

/**
 * @file RtspManager.h
 * @brief Gestor de streams RTSP. Implementa IManager.
 *
 * Gestiona:
 *   - Validación del formato de URL RTSP.
 *   - Inicio y detención de sesiones RTSP contra el backend.
 *   - Polling del estado del stream con QTimer.
 *   - Notificación del estado (StreamStatus) a los widgets.
 */
class RtspManager : public QObject, public IManager {
    Q_OBJECT

public:
    static RtspManager* instance();

    void initialize() override;
    void shutdown()   override;
    bool isInitialized() const override { return m_initialized; }

    // ── Operaciones ───────────────────────────────────────────────────
    void startStream(const QString& rtspUrl);
    void stopStream();

    Carlens::StreamStatus streamStatus() const { return m_status; }
    int                   activeSessionId() const { return m_activeSessionId; }
    const QString&        activeUrl()      const { return m_activeUrl; }

    static bool isValidRtspUrl(const QString& url);

signals:
    void streamStatusChanged(Carlens::StreamStatus status);
    void streamStarted(const SessionModel& session);
    void streamStopped();
    void streamError(const QString& reason);

private:
    explicit RtspManager(QObject* parent = nullptr);
    ~RtspManager() = default;
    RtspManager(const RtspManager&) = delete;
    RtspManager& operator=(const RtspManager&) = delete;

    void setStatus(Carlens::StreamStatus status);

    VideoService*         m_videoService;
    QTimer*               m_statusTimer;
    Carlens::StreamStatus m_status          = Carlens::StreamStatus::Disconnected;
    int                   m_activeSessionId = 0;
    QString               m_activeUrl;
    bool                  m_initialized     = false;

    static RtspManager* s_instance;

    static constexpr int STATUS_POLL_INTERVAL_MS = 5000;
};

#endif // RTSPMANAGER_H
