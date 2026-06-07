#ifndef VEHICLEMODEL_H
#define VEHICLEMODEL_H

#include <QDateTime>
#include <QDebug>
#include <QString>
#include <QVariantMap>
#include "utils/AppEnums.h"

/**
 * @file VehicleModel.h
 * @brief Modelo de datos de un vehículo detectado por el backend.
 *
 * Incluye:
 *   - Sobrecarga de operadores: == (igualdad por id), < (orden por fecha).
 *   - friend operator<<: volcado de depuración via QDebug.
 *
 * Principios: sobrecarga de operadores, friend, encapsulamiento.
 */
class VehicleModel {
public:
    VehicleModel() = default;

    // ── Getters ───────────────────────────────────────────────────────
    int                   id()           const { return m_id; }
    int                   sessionId()    const { return m_sessionId; }
    const QString&        snapshotPath() const { return m_snapshotPath; }
    const QString&        snapshotUrl()  const { return m_snapshotUrl; }
    Carlens::VehicleType  vehicleType()  const { return m_vehicleType; }
    const QString&        brand()        const { return m_brand; }
    const QString&        model()        const { return m_model; }
    const QString&        yearRange()    const { return m_yearRange; }
    const QString&        color()        const { return m_color; }
    const QString&        licensePlate() const { return m_licensePlate; }
    double                confidence()   const { return m_confidence; }
    const QDateTime&      detectedAt()   const { return m_detectedAt; }
    bool                  hasAnalysis()  const { return m_hasAnalysis; }
    bool                  isValid()      const { return m_id > 0; }

    // ── Setters ───────────────────────────────────────────────────────
    void setId(int id)                            { m_id = id; }
    void setSessionId(int sid)                    { m_sessionId = sid; }
    void setSnapshotPath(const QString& path)     { m_snapshotPath = path; }
    void setSnapshotUrl(const QString& url)       { m_snapshotUrl = url; }
    void setVehicleType(Carlens::VehicleType t)   { m_vehicleType = t; }
    void setBrand(const QString& brand)           { m_brand = brand; }
    void setModel(const QString& model)           { m_model = model; }
    void setYearRange(const QString& yearRange)   { m_yearRange = yearRange; }
    void setColor(const QString& color)           { m_color = color; }
    void setLicensePlate(const QString& lp)       { m_licensePlate = lp; }
    void setConfidence(double c)                  { m_confidence = c; }
    void setDetectedAt(const QDateTime& dt)       { m_detectedAt = dt; }
    void setHasAnalysis(bool v)                   { m_hasAnalysis = v; }

    /// Convierte el tipo enum a string legible.
    static QString vehicleTypeToString(Carlens::VehicleType type);

    /// Parsea el tipo desde string del backend.
    static Carlens::VehicleType vehicleTypeFromString(const QString& str);

    QVariantMap toMap() const;
    static VehicleModel fromMap(const QVariantMap& map);

    // ── Operadores ────────────────────────────────────────────────────
    /// Igualdad por id — útil para dedup en listas/historial.
    bool operator==(const VehicleModel& other) const;
    bool operator!=(const VehicleModel& other) const { return !(*this == other); }

    /// Orden cronológico por fecha de detección — útil para QList::sort().
    bool operator<(const VehicleModel& other) const;

    /// Volcado de depuración: qDebug() << vehicle; imprime campos clave.
    friend QDebug operator<<(QDebug dbg, const VehicleModel& v);

private:
    int                  m_id           = 0;
    int                  m_sessionId    = 0;
    QString              m_snapshotPath;
    QString              m_snapshotUrl;
    Carlens::VehicleType m_vehicleType  = Carlens::VehicleType::Unknown;
    QString              m_brand;
    QString              m_model;
    QString              m_yearRange;
    QString              m_color;
    QString              m_licensePlate;
    double               m_confidence   = 0.0;
    QDateTime            m_detectedAt;
    bool                 m_hasAnalysis  = false;
};

#endif // VEHICLEMODEL_H
