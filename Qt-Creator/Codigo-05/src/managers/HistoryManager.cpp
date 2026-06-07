#include "managers/HistoryManager.h"
#include "managers/SessionManager.h"
#include "services/adminDB.h"
#include "models/VehicleModel.h"
#include <QDebug>
#include <QtConcurrent/QtConcurrentRun>

HistoryManager::HistoryManager(QObject* parent)
    : QObject(parent)
    , m_historyWatcher(new QFutureWatcher<RemoteHistoryBackend::Result>(this))
    , m_detailWatcher(new QFutureWatcher<RemoteHistoryBackend::AnalysisDetail>(this))
{
}

void HistoryManager::initialize()
{
    if (m_initialized) return;

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
                adminDB::instance()->cacheVehicle(userId, v);
        }
        qDebug() << "[HistoryManager] Vehículos recibidos:";
        for (const VehicleModel& v : result.vehicles)
            qDebug() << v;
        emit historyLoaded(result.vehicles, result.total);
        return;
    }

    // Si el VPS falla, intentar mostrar la cache local.
    QList<VehicleModel> cached;
    if (userId > 0)
        cached = adminDB::instance()->loadCachedVehicles(userId);
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
        if (detail.analysis.isValid()) {
            qDebug() << "[HistoryManager] Análisis recibido:" << detail.analysis;
            adminDB::instance()->cacheAnalysis(detail.analysis);
        }
        emit vehicleDetailsLoaded(detail.vehicle, detail.analysis);
    } else {
        emit vehicleDetailsFailed(detail.error);
    }
}
