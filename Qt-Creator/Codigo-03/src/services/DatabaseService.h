#ifndef DATABASESERVICE_H
#define DATABASESERVICE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariantMap>
#include <QList>

#include "base/IManager.h"
#include "models/UserModel.h"
#include "models/VehicleModel.h"
#include "models/AnalysisModel.h"
#include "models/SessionModel.h"

/**
 * @file DatabaseService.h
 * @brief Servicio SQLite local. Singleton. Implementa IManager.
 *
 * Gestiona la persistencia local (caché) usando SQLite:
 *   - Sesiones de usuario (JWT, datos de usuario).
 *   - Historial de vehículos (caché offline).
 *   - Configuraciones de la aplicación (clave-valor).
 *
 * Principios aplicados:
 *   - Singleton.
 *   - Implementación de IManager (interfaz pura).
 *   - Encapsulamiento: DB solo accesible a través de esta clase.
 *   - Separación de responsabilidades: solo persistencia local.
 */
class DatabaseService : public QObject, public IManager {
    Q_OBJECT

public:
    static DatabaseService* instance();

    // ── IManager ──────────────────────────────────────────────────────
    void initialize() override;
    void shutdown()   override;
    bool isInitialized() const override { return m_initialized; }

    // ── Sesión de usuario ─────────────────────────────────────────────
    bool  saveUserSession(const UserModel& user);
    bool  loadUserSession(UserModel& outUser);
    bool  clearUserSession();
    bool  hasActiveSession() const;

    // ── Configuraciones ───────────────────────────────────────────────
    bool    saveSetting(const QString& key, const QString& value);
    QString loadSetting(const QString& key, const QString& defaultValue = {}) const;

    // ── Historial local (caché) ───────────────────────────────────────
    bool                 cacheVehicle(int userId, const VehicleModel& vehicle);
    QList<VehicleModel>  loadCachedVehicles(int userId, int limit = 50) const;
    bool                 cacheAnalysis(const AnalysisModel& analysis);
    AnalysisModel        loadCachedAnalysis(int vehicleId) const;
    bool                 clearUserVehicleCache(int userId);
    void                 clearCache();

signals:
    void databaseError(const QString& errorMsg);

private:
    explicit DatabaseService(QObject* parent = nullptr);
    ~DatabaseService() = default;
    DatabaseService(const DatabaseService&) = delete;
    DatabaseService& operator=(const DatabaseService&) = delete;

    void createTables();
    void ensureColumnExists(const QString& tableName,
                            const QString& columnName,
                            const QString& columnDefinition);
    bool execQuery(QSqlQuery& query) const;

    QSqlDatabase m_db;
    bool         m_initialized = false;

    static constexpr const char* DB_NAME = "carlens_local.db";
    static DatabaseService*      s_instance;
};

#endif // DATABASESERVICE_H
