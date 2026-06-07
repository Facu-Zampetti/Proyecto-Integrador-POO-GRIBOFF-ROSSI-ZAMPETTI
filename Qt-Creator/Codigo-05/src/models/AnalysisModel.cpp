#include "models/AnalysisModel.h"
#include <QDebug>

QVariantMap AnalysisModel::toMap() const
{
    return {
        { "id",           m_id },
        { "vehicle_id",   m_vehicleId },
        { "brand",        m_brand },
        { "model",        m_model },
        { "color",        m_color },
        { "vehicle_type", m_vehicleType },
        { "license_plate",m_licensePlate },
        { "observations", m_observations },
        { "full_text",    m_fullText },
        { "analyzed_at",  m_analyzedAt.toString(Qt::ISODate) }
    };
}

AnalysisModel AnalysisModel::fromMap(const QVariantMap& map)
{
    AnalysisModel a;
    a.setId(map.value("id").toInt());
    a.setVehicleId(map.value("vehicle_id").toInt());
    a.setBrand(map.value("brand").toString());
    a.setModel(map.value("model").toString());
    a.setColor(map.value("color").toString());
    a.setVehicleType(map.value("vehicle_type").toString());
    a.setLicensePlate(map.value("license_plate").toString());
    a.setObservations(map.value("observations").toString());
    a.setFullText(map.value("full_text").toString());
    a.setAnalyzedAt(QDateTime::fromString(map.value("analyzed_at").toString(), Qt::ISODate));
    return a;
}

// ── Operadores ─────────────────────────────────────────────────────────────

bool AnalysisModel::operator==(const AnalysisModel& other) const
{
    return m_id == other.m_id;
}

QDebug operator<<(QDebug dbg, const AnalysisModel& a)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace()
        << "AnalysisModel("
        << "id=" << a.m_id
        << ", vehicleId=" << a.m_vehicleId
        << ", brand=" << a.m_brand
        << ", model=" << a.m_model
        << ", color=" << a.m_color
        << ")";
    return dbg;
}
