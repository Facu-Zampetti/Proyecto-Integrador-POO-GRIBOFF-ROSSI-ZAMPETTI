#ifndef VIDEOMANAGER_H
#define VIDEOMANAGER_H

#include <QObject>
#include <QString>
#include "base/IManager.h"
#include "base/Singleton.h"

/**
 * @file VideoManager.h
 * @brief Gestor de validación de videos locales. Singleton via template CRTP. Implementa IManager.
 *
 * Tras eliminar el backend HTTP, el procesamiento de video ocurre localmente con
 * YOLO dentro de VideoUploadWidget (mismo pipeline que RtspWidget). VideoManager
 * deja de subir archivos: su rol pasa a validar la fuente (extensión y tamaño)
 * antes de lanzar el pipeline.
 */
class VideoManager : public QObject, public IManager, public Singleton<VideoManager> {
    Q_OBJECT

public:

    void initialize() override;
    void shutdown()   override;
    bool isInitialized() const override { return m_initialized; }

    /// Valida que el archivo exista, tenga una extensión soportada y no supere el
    /// límite de tamaño. Devuelve un mensaje de error, o cadena vacía si es válido.
    QString validateFile(const QString& filePath) const;

    static constexpr qint64 MAX_FILE_SIZE_BYTES = 500LL * 1024 * 1024; // 500 MB

private:
    friend class Singleton<VideoManager>; ///< Permite al template llamar al ctor privado.
    explicit VideoManager(QObject* parent = nullptr);
    ~VideoManager() = default;
    VideoManager(const VideoManager&) = delete;
    VideoManager& operator=(const VideoManager&) = delete;

    bool m_initialized = false;
};

#endif // VIDEOMANAGER_H
