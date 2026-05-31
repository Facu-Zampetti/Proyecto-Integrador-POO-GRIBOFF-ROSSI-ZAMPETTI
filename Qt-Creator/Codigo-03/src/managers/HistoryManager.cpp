#include "managers/HistoryManager.h"
#include "managers/SessionManager.h"
#include "services/DatabaseService.h"
#include <QDebug>

HistoryManager* HistoryManager::s_instance = nullptr;

HistoryManager* HistoryManager::instance()
{
    if (!s_instance)
        s_instance = new HistoryManager();
    return s_instance;
}

HistoryManager::HistoryManager(QObject* parent)
    : QObject(parent)
    , m_analysisService(new AnalysisService(this))
{
}

void HistoryManager::initialize()
{
    if (m_initialized) return;

    connect(m_analysisService, &AnalysisService::historyReceived,
            this, [this](const QList<VehicleModel>& vehicles, int total) {
                const int userId = SessionManager::instance()->currentUser().id();
                if (userId <= 0)
                    return;

                m_cachedHistory = vehicles;
                // Actualizar caché local
                for (const VehicleModel& v : vehicles)
                    DatabaseService::instance()->cacheVehicle(userId, v);
                emit historyLoaded(vehicles, total);
            });

    connect(m_analysisService, &AnalysisService::historyFailed,
            this, [this](const QString& reason) {
                // Intentar cargar desde caché
                const int userId = SessionManager::instance()->currentUser().id();
                QList<VehicleModel> cached;
                if (userId > 0)
                    cached = DatabaseService::instance()->loadCachedVehicles(userId);
                if (!cached.isEmpty()) {
                    qWarning() << "[HistoryManager] Backend no disponible, usando caché local.";
                    emit historyLoaded(cached, cached.size());
                } else {
                    emit historyLoadFailed(reason);
                }
            });

    connect(m_analysisService, &AnalysisService::vehicleDetailsReceived,
            this, [this](const VehicleModel& vehicle, const AnalysisModel& analysis) {
                if (analysis.isValid())
                    DatabaseService::instance()->cacheAnalysis(analysis);
                emit vehicleDetailsLoaded(vehicle, analysis);
            });

    connect(m_analysisService, &AnalysisService::vehicleDetailsFailed,
            this, [this](const QString& reason) {
                emit vehicleDetailsFailed(reason);
            });

    connect(m_analysisService, &AnalysisService::analysisReceived,
            this, [this](const AnalysisModel& analysis) {
                DatabaseService::instance()->cacheAnalysis(analysis);
                emit analysisCompleted(analysis);
            });

    connect(m_analysisService, &AnalysisService::analysisFailed,
            this, &HistoryManager::analysisFailed);

    m_initialized = true;
}

void HistoryManager::shutdown()
{
    m_initialized = false;
}

void HistoryManager::loadHistory(int page)
{
    m_analysisService->getHistory(page, 20);
}

void HistoryManager::reset()
{
    m_cachedHistory.clear();
}

void HistoryManager::loadVehicleDetails(int vehicleId)
{
    // Intentar desde caché primero
    AnalysisModel cached = DatabaseService::instance()->loadCachedAnalysis(vehicleId);
    if (cached.isValid()) {
        // Usamos datos en caché, pero igual pedimos al backend para actualizar
    }
    m_analysisService->getVehicleDetails(vehicleId);
}

void HistoryManager::requestAiAnalysis(int vehicleId)
{
    m_analysisService->requestAnalysis(vehicleId);
}
