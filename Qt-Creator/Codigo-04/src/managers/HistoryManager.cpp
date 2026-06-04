#include "managers/HistoryManager.h"
#include "managers/SessionManager.h"
#include "services/DatabaseService.h"
#include <QDebug>
#include <QtConcurrent/QtConcurrentRun>

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
    , m_historyWatcher(new QFutureWatcher<RemoteHistoryBackend::Result>(this))
    , m_detailWatcher(new QFutureWatcher<RemoteHistoryBackend::AnalysisDetail>(this))
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

    connect(m_historyWatcher, &QFutureWatcher<RemoteHistoryBackend::Result>::finished,
            this, &HistoryManager::onHistoryBackendFinished);
    connect(m_detailWatcher, &QFutureWatcher<RemoteHistoryBackend::AnalysisDetail>::finished,
            this, &HistoryManager::onDetailBackendFinished);

    m_initialized = true;
}

void HistoryManager::shutdown()
{
    m_initialized = false;
}

void HistoryManager::loadHistory(int page)
{
    const int userId = SessionManager::instance()->currentUser().id();
    if (userId <= 0) {
        emit historyLoadFailed("No hay una sesión de usuario activa.");
        return;
    }

    if (m_historyWatcher->isRunning())
        return; // Ya hay una carga en curso.

    // Lectura remota (tunel SSH + MySQL) en hilo de fondo para no congelar la UI.
    const int perPage = 20;
    QFuture<RemoteHistoryBackend::Result> future = QtConcurrent::run(
        [this, userId, page, perPage]() {
            return m_historyBackend.listHistory(userId, page, perPage);
        });
    m_historyWatcher->setFuture(future);
}

void HistoryManager::onHistoryBackendFinished()
{
    const RemoteHistoryBackend::Result result = m_historyWatcher->result();
    const int userId = SessionManager::instance()->currentUser().id();

    if (result.success) {
        m_cachedHistory = result.vehicles;
        if (userId > 0) {
            for (const VehicleModel& v : result.vehicles)
                DatabaseService::instance()->cacheVehicle(userId, v);
        }
        emit historyLoaded(result.vehicles, result.total);
        return;
    }

    // Si el VPS falla, intentar mostrar la cache local.
    QList<VehicleModel> cached;
    if (userId > 0)
        cached = DatabaseService::instance()->loadCachedVehicles(userId);
    if (!cached.isEmpty()) {
        qWarning() << "[HistoryManager] VPS no disponible, usando caché local.";
        emit historyLoaded(cached, cached.size());
    } else {
        emit historyLoadFailed(result.error);
    }
}

void HistoryManager::reset()
{
    m_cachedHistory.clear();
}

void HistoryManager::loadVehicleDetails(int vehicleId)
{
    if (m_detailWatcher->isRunning())
        return;

    // Detalle + analisis desde el VPS en hilo de fondo (no congela la UI).
    QFuture<RemoteHistoryBackend::AnalysisDetail> future = QtConcurrent::run(
        [this, vehicleId]() {
            return m_historyBackend.getAnalysis(vehicleId);
        });
    m_detailWatcher->setFuture(future);
}

void HistoryManager::onDetailBackendFinished()
{
    const RemoteHistoryBackend::AnalysisDetail detail = m_detailWatcher->result();
    if (detail.success) {
        if (detail.analysis.isValid())
            DatabaseService::instance()->cacheAnalysis(detail.analysis);
        emit vehicleDetailsLoaded(detail.vehicle, detail.analysis);
    } else {
        emit vehicleDetailsFailed(detail.error);
    }
}

void HistoryManager::requestAiAnalysis(int vehicleId)
{
    m_analysisService->requestAnalysis(vehicleId);
}
