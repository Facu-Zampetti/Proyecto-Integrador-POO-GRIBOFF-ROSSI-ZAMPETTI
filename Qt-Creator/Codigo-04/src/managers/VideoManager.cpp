#include "managers/VideoManager.h"
#include <QFileInfo>
#include <QDebug>

VideoManager* VideoManager::s_instance = nullptr;

VideoManager* VideoManager::instance()
{
    if (!s_instance)
        s_instance = new VideoManager();
    return s_instance;
}

VideoManager::VideoManager(QObject* parent)
    : QObject(parent)
    , m_videoService(new VideoService(this))
{
}

void VideoManager::initialize()
{
    if (m_initialized) return;

    connect(m_videoService, &VideoService::uploadStarted,
            this, [this](const QString& filename) {
                m_uploading = true;
                emit uploadStarted(filename);
            });

    connect(m_videoService, &VideoService::uploadProgressUpdated,
            this, &VideoManager::uploadProgressChanged);

    connect(m_videoService, &VideoService::uploadFinished,
            this, [this](const SessionModel& session) {
                m_uploading = false;
                emit uploadCompleted(session);
            });

    connect(m_videoService, &VideoService::uploadFailed,
            this, [this](const QString& reason) {
                m_uploading = false;
                emit uploadFailed(reason);
            });

    m_initialized = true;
}

void VideoManager::shutdown()
{
    m_initialized = false;
}

void VideoManager::uploadVideo(const QString& filePath)
{
    if (m_uploading) {
        emit uploadFailed("Ya hay una carga en progreso.");
        return;
    }

    QFileInfo fi(filePath);
    if (!fi.exists()) {
        emit uploadFailed("El archivo no existe.");
        return;
    }

    // Validar extensión
    const QString ext = fi.suffix().toLower();
    if (ext != "mp4" && ext != "avi" && ext != "mkv") {
        emit uploadFailed("Formato no soportado. Use MP4, AVI o MKV.");
        return;
    }

    // Validar tamaño
    if (fi.size() > MAX_FILE_SIZE_BYTES) {
        emit uploadFailed(
            QString("El archivo supera el límite de %1 MB.")
            .arg(MAX_FILE_SIZE_BYTES / (1024 * 1024)));
        return;
    }

    m_videoService->uploadVideo(filePath);
}

void VideoManager::cancelUpload()
{
    // Nota: QNetworkAccessManager no tiene cancel directo per-reply
    // en esta arquitectura. Se podría extender ApiClient para soportarlo.
    m_uploading = false;
    qWarning() << "[VideoManager] Cancelación de carga no implementada completamente.";
}
