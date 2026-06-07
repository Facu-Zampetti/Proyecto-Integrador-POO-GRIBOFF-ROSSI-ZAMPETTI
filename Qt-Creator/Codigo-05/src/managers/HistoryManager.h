#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include <QObject>
#include <QList>
#include <QFutureWatcher>
#include "base/IManager.h"
#include "base/Singleton.h"
#include "models/VehicleModel.h"
#include "models/AnalysisModel.h"
#include "services/RemoteHistoryBackend.h"

/**
 * @file HistoryManager.h
 * @brief Gestor del historial de vehículos. Singleton via template CRTP. Implementa IManager.
 *
 * Orquesta:
 *   - Carga del historial desde la MySQL del VPS o caché SQLite (offline).
 *   - Detalle + análisis de un vehículo concreto.
 *   - Caché local de resultados.
 */
class HistoryManager : public QObject, public IManager, public Singleton<HistoryManager> {
    Q_OBJECT

public:

    void initialize() override;
    void shutdown()   override;
    bool isInitialized() const override { return m_initialized; }

    // ── Operaciones ───────────────────────────────────────────────────
    void loadHistory(int page = 1);
    void loadVehicleDetails(int vehicleId);
    void reset();

    const QList<VehicleModel>& cachedHistory() const { return m_cachedHistory; }

signals:
    void historyLoaded(const QList<VehicleModel>& vehicles, int total);
    void historyLoadFailed(const QString& reason);
    void vehicleDetailsLoaded(const VehicleModel& vehicle, const AnalysisModel& analysis);
    void vehicleDetailsFailed(const QString& reason);

private:
    friend class Singleton<HistoryManager>; ///< Permite al template llamar al ctor privado.
    explicit HistoryManager(QObject* parent = nullptr);
    ~HistoryManager() = default;
    HistoryManager(const HistoryManager&) = delete;
    HistoryManager& operator=(const HistoryManager&) = delete;

    void onHistoryBackendFinished();
    void onDetailBackendFinished();

    QList<VehicleModel> m_cachedHistory;
    bool                m_initialized = false;

    // Historial / detalle leidos desde la MySQL del VPS en hilo de fondo.
    RemoteHistoryBackend                                  m_historyBackend;
    QFutureWatcher<RemoteHistoryBackend::Result>*         m_historyWatcher = nullptr;
    QFutureWatcher<RemoteHistoryBackend::AnalysisDetail>* m_detailWatcher  = nullptr;
};

#endif // HISTORYMANAGER_H
