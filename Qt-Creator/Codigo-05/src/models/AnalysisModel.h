#ifndef ANALYSISMODEL_H
#define ANALYSISMODEL_H

#include <QDebug>
#include <QString>
#include <QDateTime>
#include <QVariantMap>

/**
 * @file AnalysisModel.h
 * @brief Modelo de datos del análisis IA (Gemini) de un vehículo.
 *
 * Incluye:
 *   - Sobrecarga de operadores: == (igualdad por id).
 *   - friend operator<<: volcado de depuración via QDebug.
 *
 * Principios: sobrecarga de operadores, friend, encapsulamiento.
 */
class AnalysisModel {
public:
    AnalysisModel() = default;

    // ── Getters ───────────────────────────────────────────────────────
    int            id()             const { return m_id; }
    int            vehicleId()      const { return m_vehicleId; }
    const QString& brand()          const { return m_brand; }
    const QString& model()          const { return m_model; }
    const QString& color()          const { return m_color; }
    const QString& vehicleType()    const { return m_vehicleType; }
    const QString& licensePlate()   const { return m_licensePlate; }
    const QString& observations()   const { return m_observations; }
    const QString& fullText()       const { return m_fullText; }
    const QDateTime& analyzedAt()   const { return m_analyzedAt; }
    bool           isValid()        const { return m_id > 0 && m_vehicleId > 0; }

    // ── Setters ───────────────────────────────────────────────────────
    void setId(int id)                           { m_id = id; }
    void setVehicleId(int vid)                   { m_vehicleId = vid; }
    void setBrand(const QString& s)              { m_brand = s; }
    void setModel(const QString& s)              { m_model = s; }
    void setColor(const QString& s)              { m_color = s; }
    void setVehicleType(const QString& s)        { m_vehicleType = s; }
    void setLicensePlate(const QString& s)       { m_licensePlate = s; }
    void setObservations(const QString& s)       { m_observations = s; }
    void setFullText(const QString& s)           { m_fullText = s; }
    void setAnalyzedAt(const QDateTime& dt)      { m_analyzedAt = dt; }

    QVariantMap toMap() const;
    static AnalysisModel fromMap(const QVariantMap& map);

    // ── Operadores ────────────────────────────────────────────────────
    /// Igualdad por id — útil para evitar duplicados en caché.
    bool operator==(const AnalysisModel& other) const;

    /// Volcado de depuración: qDebug() << analysis; imprime campos clave.
    friend QDebug operator<<(QDebug dbg, const AnalysisModel& a);

private:
    int       m_id          = 0;
    int       m_vehicleId   = 0;
    QString   m_brand;
    QString   m_model;
    QString   m_color;
    QString   m_vehicleType;
    QString   m_licensePlate;
    QString   m_observations;
    QString   m_fullText;
    QDateTime m_analyzedAt;
};

#endif // ANALYSISMODEL_H
