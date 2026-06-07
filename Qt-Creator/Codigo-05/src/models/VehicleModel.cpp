#include "models/VehicleModel.h"
#include <QDebug>

QString VehicleModel::vehicleTypeToString(Carlens::VehicleType type)
{
    switch (type) {
    case Carlens::VehicleType::Car:        return "Automóvil";
    case Carlens::VehicleType::Truck:      return "Camión";
    case Carlens::VehicleType::Bus:        return "Autobús";
    case Carlens::VehicleType::Motorcycle: return "Motocicleta";
    case Carlens::VehicleType::Van:        return "Camioneta";
    case Carlens::VehicleType::Bicycle:    return "Bicicleta";
    default:                               return "Desconocido";
    }
}

Carlens::VehicleType VehicleModel::vehicleTypeFromString(const QString& str)
{
    const QString lower = str.toLower();
    if (lower == "car")        return Carlens::VehicleType::Car;
    if (lower == "truck")      return Carlens::VehicleType::Truck;
    if (lower == "bus")        return Carlens::VehicleType::Bus;
    if (lower == "motorcycle") return Carlens::VehicleType::Motorcycle;
    if (lower == "van")        return Carlens::VehicleType::Van;
    if (lower == "bicycle")    return Carlens::VehicleType::Bicycle;
    return Carlens::VehicleType::Unknown;
}

QVariantMap VehicleModel::toMap() const
{
    return {
        { "id",            m_id },
        { "session_id",    m_sessionId },
        { "snapshot_path", m_snapshotPath },
        { "snapshot_url",  m_snapshotUrl },
        { "vehicle_type",  static_cast<int>(m_vehicleType) },
        { "brand",         m_brand },
        { "model",         m_model },
        { "year_range",    m_yearRange },
        { "color",         m_color },
        { "license_plate", m_licensePlate },
        { "confidence",    m_confidence },
        { "detected_at",   m_detectedAt.toString(Qt::ISODate) },
        { "has_analysis",  m_hasAnalysis ? 1 : 0 }
    };
}

// ── Operadores ─────────────────────────────────────────────────────────────

bool VehicleModel::operator==(const VehicleModel& other) const
{
    return m_id == other.m_id;
}

bool VehicleModel::operator<(const VehicleModel& other) const
{
    return m_detectedAt < other.m_detectedAt;
}

QDebug operator<<(QDebug dbg, const VehicleModel& v)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace()
        << "VehicleModel("
        << "id=" << v.m_id
        << ", type=" << VehicleModel::vehicleTypeToString(v.m_vehicleType)
        << ", brand=" << v.m_brand
        << ", confidence=" << v.m_confidence
        << ", detectedAt=" << v.m_detectedAt.toString(Qt::ISODate)
        << ")";
    return dbg;
}

VehicleModel VehicleModel::fromMap(const QVariantMap& map)
{
    VehicleModel v;
    v.setId(map.value("id").toInt());
    v.setSessionId(map.value("session_id").toInt());
    v.setSnapshotPath(map.value("snapshot_path").toString());
    v.setSnapshotUrl(map.value("snapshot_url").toString());
    v.setVehicleType(static_cast<Carlens::VehicleType>(map.value("vehicle_type").toInt()));
    v.setBrand(map.value("brand").toString());
    v.setModel(map.value("model").toString());
    v.setYearRange(map.value("year_range").toString());
    v.setColor(map.value("color").toString());
    v.setLicensePlate(map.value("license_plate").toString());
    v.setConfidence(map.value("confidence").toDouble());
    v.setDetectedAt(QDateTime::fromString(map.value("detected_at").toString(), Qt::ISODate));
    v.setHasAnalysis(map.value("has_analysis").toInt() == 1);
    return v;
}
