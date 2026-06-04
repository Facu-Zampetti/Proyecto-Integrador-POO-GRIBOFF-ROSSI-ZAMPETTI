#ifndef ANALYSISSERVICE_H
#define ANALYSISSERVICE_H

#include "base/AbstractApiService.h"
#include "models/VehicleModel.h"
#include "models/AnalysisModel.h"
#include <QList>

/**
 * @file AnalysisService.h
 * @brief Servicio de análisis IA y consulta de historial. Hereda de AbstractApiService.
 *
 * Gestiona:
 *  - Solicitud de análisis Gemini para un vehículo.
 *  - Obtención del historial de vehículos detectados.
 *  - Obtención de detalles de un vehículo específico.
 */
class AnalysisService : public AbstractApiService {
    Q_OBJECT

public:
    explicit AnalysisService(QObject* parent = nullptr);

    // ── Operaciones públicas ───────────────────────────────────────────
    void requestAnalysis(int vehicleId);
    void getHistory(int page = 1, int perPage = 20);
    void getVehicleDetails(int vehicleId);
    void getAnalysis(int vehicleId);

    // ── Endpoints ─────────────────────────────────────────────────────
    static constexpr const char* EP_ANALYSIS_BASE = "/api/analysis";
    static constexpr const char* EP_HISTORY        = "/api/vehicles";

    void get(const QString& endpoint) override;
    void post(const QString& endpoint, const QJsonObject& payload) override;

signals:
    void analysisReceived(const AnalysisModel& analysis);
    void analysisFailed(const QString& reason);
    void historyReceived(const QList<VehicleModel>& vehicles, int total);
    void historyFailed(const QString& reason);
    void vehicleDetailsReceived(const VehicleModel& vehicle,
                                const AnalysisModel& analysis);
    void vehicleDetailsFailed(const QString& reason);

protected:
    void handleSuccess(const QString& endpoint,
                       const QJsonDocument& response) override;

    void handleError(const QString& endpoint,
                     const QString& errorMessage,
                     int httpStatusCode) override;

private:
    VehicleModel  parseVehicle(const QJsonObject& obj) const;
    AnalysisModel parseAnalysis(const QJsonObject& obj) const;
};

#endif // ANALYSISSERVICE_H
