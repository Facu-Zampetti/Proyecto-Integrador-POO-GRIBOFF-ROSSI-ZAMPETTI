#include "managers/RtspManager.h"
#include <QRegularExpression>
#include <QDebug>

RtspManager* RtspManager::s_instance = nullptr;

RtspManager* RtspManager::instance()
{
    if (!s_instance)
        s_instance = new RtspManager();
    return s_instance;
}

RtspManager::RtspManager(QObject* parent)
    : QObject(parent)
    , m_videoService(new VideoService(this))
    , m_statusTimer(new QTimer(this))
{
    m_statusTimer->setInterval(STATUS_POLL_INTERVAL_MS);
}

void RtspManager::initialize()
{
    if (m_initialized) return;

    connect(m_videoService, &VideoService::rtspStarted,
            this, [this](const SessionModel& session) {
                m_activeSessionId = session.id();
                setStatus(Carlens::StreamStatus::Connected);
                m_statusTimer->start();
                emit streamStarted(session);
            });

    connect(m_videoService, &VideoService::rtspStartFailed,
            this, [this](const QString& reason) {
                setStatus(Carlens::StreamStatus::Error);
                emit streamError(reason);
            });

    connect(m_videoService, &VideoService::rtspStopped,
            this, [this]() {
                m_activeSessionId = 0;
                m_activeUrl.clear();
                m_statusTimer->stop();
                setStatus(Carlens::StreamStatus::Disconnected);
                emit streamStopped();
            });

    connect(m_videoService, &VideoService::rtspStopFailed,
            this, [this](const QString& reason) {
                emit streamError("Error al detener stream: " + reason);
            });

    connect(m_videoService, &VideoService::sessionStatusReceived,
            this, [this](const SessionModel& session) {
                if (session.id() != m_activeSessionId || m_activeSessionId <= 0)
                    return;

                switch (session.uploadStatus()) {
                case Carlens::UploadStatus::Processing:
                    setStatus(Carlens::StreamStatus::Connected);
                    break;
                case Carlens::UploadStatus::Completed:
                    m_activeSessionId = 0;
                    m_activeUrl.clear();
                    m_statusTimer->stop();
                    setStatus(Carlens::StreamStatus::Disconnected);
                    emit streamStopped();
                    break;
                case Carlens::UploadStatus::Failed:
                    m_activeSessionId = 0;
                    m_activeUrl.clear();
                    m_statusTimer->stop();
                    setStatus(Carlens::StreamStatus::Error);
                    emit streamError("El backend no pudo continuar el procesamiento RTSP.");
                    break;
                default:
                    break;
                }
            });

    connect(m_statusTimer, &QTimer::timeout,
            this, [this]() {
                if (m_activeSessionId > 0)
                    m_videoService->getSessionStatus(m_activeSessionId);
            });

    m_initialized = true;
}

void RtspManager::shutdown()
{
    if (m_status == Carlens::StreamStatus::Connected)
        stopStream();
    m_statusTimer->stop();
    m_initialized = false;
}

void RtspManager::startStream(const QString& rtspUrl)
{
    if (!isValidRtspUrl(rtspUrl)) {
        emit streamError("URL RTSP inválida. Formato esperado: rtsp://host:port/path");
        return;
    }

    if (m_status == Carlens::StreamStatus::Connected) {
        emit streamError("Ya hay un stream activo. Deténgalo primero.");
        return;
    }

    m_activeUrl = rtspUrl;
    setStatus(Carlens::StreamStatus::Connecting);
    m_videoService->startRtspSession(rtspUrl);
}

void RtspManager::stopStream()
{
    if (m_activeSessionId > 0)
        m_videoService->stopRtspSession(m_activeSessionId);
}

bool RtspManager::isValidRtspUrl(const QString& url)
{
    static const QRegularExpression rtspRegex(
        R"(^rtsp://[\w\.\-]+(:\d+)?(/[\w\.\-/]*)?$)",
        QRegularExpression::CaseInsensitiveOption
    );
    return rtspRegex.match(url).hasMatch();
}

void RtspManager::setStatus(Carlens::StreamStatus status)
{
    if (m_status != status) {
        m_status = status;
        emit streamStatusChanged(status);
    }
}
