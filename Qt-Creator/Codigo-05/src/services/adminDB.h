#ifndef ADMINDB_H
#define ADMINDB_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariantMap>
#include <QList>

#include "base/IManager.h"
#include "base/Singleton.h"
#include "models/UserModel.h"
#include "models/VehicleModel.h"
#include "models/AnalysisModel.h"
#include "models/SessionModel.h"

/**
 * @file adminDB.h
 * @brief Servicio SQLite local. Singleton. Implementa IManager.
 *
 * Gestiona la persistencia local (caché) usando SQLite:
 *   - Sesiones de usuario (JWT, datos de usuario).
 *   - Historial de vehículos (caché offline).
 *   - Configuraciones de la aplicación (clave-valor).
 *
 * Principios aplicados:
 *   - Singleton template CRTP (Singleton<adminDB>).
 *   - Implementación de IManager (interfaz pura).
 *   - Encapsulamiento: DB solo accesible a través de esta clase.
 *   - Separación de responsabilidades: solo persistencia local.
 */
class adminDB : public QObject, public IManager, public Singleton<adminDB> {
    Q_OBJECT

public:

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

    // ── ODBC (demo aislado — no afecta SQLite ni el startup) ─────────
    /**
     * @brief Intenta abrir una conexión ODBC al DSN indicado.
     *
     * Uso de demostración académica. Si el driver QODBC o el DSN no están
     * disponibles, retorna false sin lanzar excepciones ni afectar la DB
     * SQLite principal.
     *
     * Configuración: crear un DSN ODBC de sistema/usuario en Windows
     * (Panel de Control → Herramientas Administrativas → Orígenes de datos ODBC).
     *
     * @param dsn  Nombre del DSN ODBC configurado en el sistema.
     * @return true si la conexión se abrió correctamente.
     */
    bool tryOpenOdbc(const QString& dsn);

    /** Intenta abrir el DSN ODBC y retorna un Result rico con el diagnóstico. */
    Result<QString> probeOdbc(const QString& dsn);

signals:
    void databaseError(const QString& errorMsg);

private:
    friend class Singleton<adminDB>; ///< Permite al template llamar al ctor privado.
    explicit adminDB(QObject* parent = nullptr);
    ~adminDB() = default;
    adminDB(const adminDB&) = delete;
    adminDB& operator=(const adminDB&) = delete;

    void createTables();
    void ensureColumnExists(const QString& tableName,
                            const QString& columnName,
                            const QString& columnDefinition);
    bool execQuery(QSqlQuery& query) const;

    QSqlDatabase m_db;
    bool         m_initialized = false;

    static constexpr const char* DB_NAME = "carlens_local.db";
};

#endif // ADMINDB_H
