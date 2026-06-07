#include "models/SessionModel.h"

QVariantMap SessionModel::toMap() const
{
    return {
        { "id",             m_id },
        { "user_id",        m_userId },
        { "source_type",    m_sourceType },
        { "source_url",     m_sourceUrl },
        { "filename",       m_filename },
        { "total_vehicles", m_totalVehicles },
        { "started_at",     m_startedAt.toString(Qt::ISODate) },
        { "ended_at",       m_endedAt.toString(Qt::ISODate) },
        { "upload_status",  static_cast<int>(m_uploadStatus) }
    };
}

SessionModel SessionModel::fromMap(const QVariantMap& map)
{
    SessionModel s;
    s.setId(map.value("id").toInt());
    s.setUserId(map.value("user_id").toInt());
    s.setSourceType(map.value("source_type").toString());
    s.setSourceUrl(map.value("source_url").toString());
    s.setFilename(map.value("filename").toString());
    s.setTotalVehicles(map.value("total_vehicles").toInt());
    s.setStartedAt(QDateTime::fromString(map.value("started_at").toString(), Qt::ISODate));
    s.setEndedAt(QDateTime::fromString(map.value("ended_at").toString(), Qt::ISODate));
    s.setUploadStatus(static_cast<Carlens::UploadStatus>(map.value("upload_status").toInt()));
    return s;
}
