#include "managers/VideoManager.h"
#include <QFileInfo>

VideoManager::VideoManager(QObject* parent)
    : QObject(parent)
{
}

void VideoManager::initialize()
{
    m_initialized = true;
}

void VideoManager::shutdown()
{
    m_initialized = false;
}

QString VideoManager::validateFile(const QString& filePath) const
{
    QFileInfo fi(filePath);
    if (!fi.exists())
        return QStringLiteral("El archivo no existe.");

    const QString ext = fi.suffix().toLower();
    if (ext != "mp4" && ext != "avi" && ext != "mkv")
        return QStringLiteral("Formato no soportado. Use MP4, AVI o MKV.");

    if (fi.size() > MAX_FILE_SIZE_BYTES)
        return QString("El archivo supera el límite de %1 MB.")
            .arg(MAX_FILE_SIZE_BYTES / (1024 * 1024));

    return QString(); // válido
}
