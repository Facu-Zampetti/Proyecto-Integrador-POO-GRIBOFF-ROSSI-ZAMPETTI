#ifndef VIDEOMANAGER_H
#define VIDEOMANAGER_H

#include <QObject>
#include "base/IManager.h"
#include "models/SessionModel.h"
#include "services/VideoService.h"

/**
 * @file VideoManager.h
 * @brief Gestor de carga de videos MP4. Implementa IManager.
 *
 * Orquesta el flujo completo de subida de video:
 *   - Validar extensión y tamaño del archivo.
 *   - Delegar la subida al VideoService.
 *   - Reportar progreso y resultado a los widgets.
 */
class VideoManager : public QObject, public IManager {
    Q_OBJECT

public:
    static VideoManager* instance();

    void initialize() override;
    void shutdown()   override;
    bool isInitialized() const override { return m_initialized; }

    void uploadVideo(const QString& filePath);
    void cancelUpload();

    bool isUploading() const { return m_uploading; }

    static constexpr qint64 MAX_FILE_SIZE_BYTES = 500LL * 1024 * 1024; // 500 MB

signals:
    void uploadStarted(const QString& filename);
    void uploadProgressChanged(int percent);
    void uploadCompleted(const SessionModel& session);
    void uploadFailed(const QString& reason);

private:
    explicit VideoManager(QObject* parent = nullptr);
    ~VideoManager() = default;
    VideoManager(const VideoManager&) = delete;
    VideoManager& operator=(const VideoManager&) = delete;

    VideoService* m_videoService;
    bool          m_initialized = false;
    bool          m_uploading   = false;

    static VideoManager* s_instance;
};

#endif // VIDEOMANAGER_H
