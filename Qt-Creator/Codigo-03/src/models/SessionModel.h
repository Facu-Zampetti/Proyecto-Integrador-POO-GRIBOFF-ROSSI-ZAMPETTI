#ifndef SESSIONMODEL_H
#define SESSIONMODEL_H

#include <QString>
#include <QDateTime>
#include <QVariantMap>
#include "utils/AppEnums.h"

/**
 * @file SessionModel.h
 * @brief Modelo que representa una sesión de análisis de video.
 *
 * Una sesión corresponde a un video MP4 cargado o un stream RTSP
 * procesado en el backend. Agrupa los vehículos detectados.
 */
class SessionModel {
public:
    SessionModel() = default;

    // ── Getters ───────────────────────────────────────────────────────
    int                    id()           const { return m_id; }
    int                    userId()       const { return m_userId; }
    const QString&         sourceType()   const { return m_sourceType; }
    const QString&         sourceUrl()    const { return m_sourceUrl; }
    const QString&         filename()     const { return m_filename; }
    int                    totalVehicles() const { return m_totalVehicles; }
    const QDateTime&       startedAt()    const { return m_startedAt; }
    const QDateTime&       endedAt()      const { return m_endedAt; }
    Carlens::UploadStatus  uploadStatus() const { return m_uploadStatus; }
    bool                   isValid()      const { return m_id > 0; }

    // ── Setters ───────────────────────────────────────────────────────
    void setId(int id)                            { m_id = id; }
    void setUserId(int uid)                       { m_userId = uid; }
    void setSourceType(const QString& t)          { m_sourceType = t; }
    void setSourceUrl(const QString& url)         { m_sourceUrl = url; }
    void setFilename(const QString& fn)           { m_filename = fn; }
    void setTotalVehicles(int n)                  { m_totalVehicles = n; }
    void setStartedAt(const QDateTime& dt)        { m_startedAt = dt; }
    void setEndedAt(const QDateTime& dt)          { m_endedAt = dt; }
    void setUploadStatus(Carlens::UploadStatus s) { m_uploadStatus = s; }

    QVariantMap toMap() const;
    static SessionModel fromMap(const QVariantMap& map);

private:
    int                   m_id            = 0;
    int                   m_userId        = 0;
    QString               m_sourceType;   // "mp4" | "rtsp"
    QString               m_sourceUrl;
    QString               m_filename;
    int                   m_totalVehicles = 0;
    QDateTime             m_startedAt;
    QDateTime             m_endedAt;
    Carlens::UploadStatus m_uploadStatus  = Carlens::UploadStatus::Idle;
};

#endif // SESSIONMODEL_H
