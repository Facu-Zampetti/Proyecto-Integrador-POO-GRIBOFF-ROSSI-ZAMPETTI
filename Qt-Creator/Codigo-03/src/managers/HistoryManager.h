#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include <QObject>
#include <QList>
#include "base/IManager.h"
#include "models/VehicleModel.h"
#include "models/AnalysisModel.h"
#include "services/AnalysisService.h"

/**
 * @file HistoryManager.h
 * @brief Gestor del historial de vehículos. Implementa IManager.
 *
 * Orquesta:
 *   - Carga del historial desde el backend (online) o caché SQLite (offline).
 *   - Solicitud de análisis IA para un vehículo.
 *   - Caché local de resultados.
 */
class HistoryManager : public QObject, public IManager {
    Q_OBJECT

public:
    static HistoryManager* instance();

    void initialize() override;
    void shutdown()   override;
    bool isInitialized() const override { return m_initialized; }

    // ── Operaciones ───────────────────────────────────────────────────
    void loadHistory(int page = 1);
    void loadVehicleDetails(int vehicleId);
    void requestAiAnalysis(int vehicleId);
    void reset();

    const QList<VehicleModel>& cachedHistory() const { return m_cachedHistory; }

signals:
    void historyLoaded(const QList<VehicleModel>& vehicles, int total);
    void historyLoadFailed(const QString& reason);
    void vehicleDetailsLoaded(const VehicleModel& vehicle, const AnalysisModel& analysis);
    void vehicleDetailsFailed(const QString& reason);
    void analysisCompleted(const AnalysisModel& analysis);
    void analysisFailed(const QString& reason);

private:
    explicit HistoryManager(QObject* parent = nullptr);
    ~HistoryManager() = default;
    HistoryManager(const HistoryManager&) = delete;
    HistoryManager& operator=(const HistoryManager&) = delete;

    AnalysisService*    m_analysisService;
    QList<VehicleModel> m_cachedHistory;
    bool                m_initialized = false;

    static HistoryManager* s_instance;
};

#endif // HISTORYMANAGER_H
