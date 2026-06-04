#include "services/DatabaseService.h"
#include <QSqlError>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QSet>

DatabaseService* DatabaseService::s_instance = nullptr;

DatabaseService* DatabaseService::instance()
{
    if (!s_instance)
        s_instance = new DatabaseService();
    return s_instance;
}

DatabaseService::DatabaseService(QObject* parent)
    : QObject(parent)
{
}

// ─── IManager ─────────────────────────────────────────────────────────────

void DatabaseService::initialize()
{
    if (m_initialized) return;

    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataPath);
    QString dbPath = dataPath + "/" + DB_NAME;

    m_db = QSqlDatabase::addDatabase("QSQLITE", "carlens_connection");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        QString err = m_db.lastError().text();
        qCritical() << "[DatabaseService] No se pudo abrir la base de datos:" << err;
        emit databaseError("No se pudo abrir la base de datos local: " + err);
        return;
    }

    createTables();
    m_initialized = true;
    qInfo() << "[DatabaseService] Base de datos inicializada en:" << dbPath;
}

void DatabaseService::shutdown()
{
    if (m_db.isOpen())
        m_db.close();
    QSqlDatabase::removeDatabase("carlens_connection");
    m_initialized = false;
}

void DatabaseService::createTables()
{
    QSqlQuery q(m_db);

    // Tabla de sesión de usuario (recordar sesión)
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS user_session (
            id         INTEGER PRIMARY KEY,
            user_id    INTEGER NOT NULL,
            username   TEXT    NOT NULL,
            email      TEXT    NOT NULL,
            token      TEXT    NOT NULL,
            saved_at   TEXT    NOT NULL
        )
    )");

    // Tabla de configuraciones clave-valor
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS app_settings (
            key   TEXT PRIMARY KEY,
            value TEXT NOT NULL
        )
    )");

    // Tabla de caché de vehículos
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS cached_vehicles (
            id             INTEGER PRIMARY KEY,
            user_id        INTEGER NOT NULL DEFAULT 0,
            session_id     INTEGER,
            snapshot_path  TEXT,
            snapshot_url   TEXT,
            vehicle_type   INTEGER,
            brand          TEXT,
            model          TEXT,
            year_range     TEXT,
            color          TEXT,
            license_plate  TEXT,
            confidence     REAL,
            detected_at    TEXT,
            has_analysis   INTEGER DEFAULT 0
        )
    )");

    // Tabla de caché de análisis IA
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS cached_analysis (
            id           INTEGER PRIMARY KEY,
            vehicle_id   INTEGER NOT NULL UNIQUE,
            brand        TEXT,
            model        TEXT,
            color        TEXT,
            vehicle_type TEXT,
            license_plate TEXT,
            observations TEXT,
            full_text    TEXT,
            analyzed_at  TEXT
        )
    )");

    ensureColumnExists("cached_vehicles", "user_id", "INTEGER NOT NULL DEFAULT 0");
    ensureColumnExists("cached_vehicles", "year_range", "TEXT");
}

void DatabaseService::ensureColumnExists(const QString& tableName,
                                         const QString& columnName,
                                         const QString& columnDefinition)
{
    QSqlQuery query(m_db);
    if (!query.exec(QString("PRAGMA table_info(%1)").arg(tableName)))
        return;

    QSet<QString> columns;
    while (query.next())
        columns.insert(query.value("name").toString());

    if (columns.contains(columnName))
        return;

    QSqlQuery alter(m_db);
    alter.exec(QString("ALTER TABLE %1 ADD COLUMN %2 %3")
                   .arg(tableName, columnName, columnDefinition));
}

bool DatabaseService::execQuery(QSqlQuery& query) const
{
    if (!query.exec()) {
        qWarning() << "[DatabaseService] Error SQL:" << query.lastError().text();
        return false;
    }
    return true;
}

// ─── Sesión de usuario ────────────────────────────────────────────────────

bool DatabaseService::saveUserSession(const UserModel& user)
{
    QSqlQuery q(m_db);
    q.exec("DELETE FROM user_session");

    q.prepare(R"(
        INSERT INTO user_session (user_id, username, email, token, saved_at)
        VALUES (:uid, :uname, :email, :token, :saved)
    )");
    q.bindValue(":uid",   user.id());
    q.bindValue(":uname", user.username());
    q.bindValue(":email", user.email());
    q.bindValue(":token", user.token());
    q.bindValue(":saved", QDateTime::currentDateTime().toString(Qt::ISODate));

    return execQuery(q);
}

bool DatabaseService::loadUserSession(UserModel& outUser)
{
    QSqlQuery q(m_db);
    q.exec("SELECT user_id, username, email, token FROM user_session LIMIT 1");

    if (q.next()) {
        outUser = UserModel(
            q.value("user_id").toInt(),
            q.value("username").toString(),
            q.value("email").toString(),
            q.value("token").toString()
        );
        return true;
    }
    return false;
}

bool DatabaseService::clearUserSession()
{
    QSqlQuery q(m_db);
    return q.exec("DELETE FROM user_session");
}

bool DatabaseService::hasActiveSession() const
{
    QSqlQuery q(m_db);
    q.exec("SELECT COUNT(*) FROM user_session");
    if (q.next())
        return q.value(0).toInt() > 0;
    return false;
}

// ─── Configuraciones ──────────────────────────────────────────────────────

bool DatabaseService::saveSetting(const QString& key, const QString& value)
{
    QSqlQuery q(m_db);
    q.prepare(R"(
        INSERT OR REPLACE INTO app_settings (key, value)
        VALUES (:key, :val)
    )");
    q.bindValue(":key", key);
    q.bindValue(":val", value);
    return execQuery(q);
}

QString DatabaseService::loadSetting(const QString& key,
                                      const QString& defaultValue) const
{
    QSqlQuery q(m_db);
    q.prepare("SELECT value FROM app_settings WHERE key = :key");
    q.bindValue(":key", key);
    if (q.exec() && q.next())
        return q.value(0).toString();
    return defaultValue;
}

// ─── Caché de vehículos ───────────────────────────────────────────────────

bool DatabaseService::cacheVehicle(int userId, const VehicleModel& v)
{
    QSqlQuery q(m_db);
    q.prepare(R"(
        INSERT OR REPLACE INTO cached_vehicles
        (id, user_id, session_id, snapshot_path, snapshot_url, vehicle_type,
            brand, model, year_range, color, license_plate, confidence, detected_at, has_analysis)
        VALUES
        (:id, :uid, :sid, :spath, :surl, :vtype,
            :brand, :model, :year, :color, :lp, :conf, :dat, :ha)
    )");
    q.bindValue(":id",    v.id());
    q.bindValue(":uid",   userId);
    q.bindValue(":sid",   v.sessionId());
    q.bindValue(":spath", v.snapshotPath());
    q.bindValue(":surl",  v.snapshotUrl());
    q.bindValue(":vtype", static_cast<int>(v.vehicleType()));
    q.bindValue(":brand", v.brand());
    q.bindValue(":model", v.model());
    q.bindValue(":year",  v.yearRange());
    q.bindValue(":color", v.color());
    q.bindValue(":lp",    v.licensePlate());
    q.bindValue(":conf",  v.confidence());
    q.bindValue(":dat",   v.detectedAt().toString(Qt::ISODate));
    q.bindValue(":ha",    v.hasAnalysis() ? 1 : 0);
    return execQuery(q);
}

QList<VehicleModel> DatabaseService::loadCachedVehicles(int userId, int limit) const
{
    QList<VehicleModel> result;
    QSqlQuery q(m_db);
    q.prepare(
        "SELECT * FROM cached_vehicles WHERE user_id = :uid "
        "ORDER BY detected_at DESC LIMIT :lim");
    q.bindValue(":uid", userId);
    q.bindValue(":lim", limit);
    if (!q.exec()) return result;

    while (q.next()) {
        QVariantMap map;
        for (int i = 0; i < q.record().count(); ++i)
            map[q.record().fieldName(i)] = q.value(i);
        result.append(VehicleModel::fromMap(map));
    }
    return result;
}

bool DatabaseService::cacheAnalysis(const AnalysisModel& a)
{
    QSqlQuery q(m_db);
    q.prepare(R"(
        INSERT OR REPLACE INTO cached_analysis
        (id, vehicle_id, brand, model, color, vehicle_type,
         license_plate, observations, full_text, analyzed_at)
        VALUES (:id, :vid, :brand, :model, :color, :vtype,
                :lp, :obs, :ft, :aat)
    )");
    q.bindValue(":id",    a.id());
    q.bindValue(":vid",   a.vehicleId());
    q.bindValue(":brand", a.brand());
    q.bindValue(":model", a.model());
    q.bindValue(":color", a.color());
    q.bindValue(":vtype", a.vehicleType());
    q.bindValue(":lp",    a.licensePlate());
    q.bindValue(":obs",   a.observations());
    q.bindValue(":ft",    a.fullText());
    q.bindValue(":aat",   a.analyzedAt().toString(Qt::ISODate));
    return execQuery(q);
}

AnalysisModel DatabaseService::loadCachedAnalysis(int vehicleId) const
{
    QSqlQuery q(m_db);
    q.prepare("SELECT * FROM cached_analysis WHERE vehicle_id = :vid LIMIT 1");
    q.bindValue(":vid", vehicleId);
    if (!q.exec() || !q.next()) return AnalysisModel{};

    QVariantMap map;
    for (int i = 0; i < q.record().count(); ++i)
        map[q.record().fieldName(i)] = q.value(i);
    return AnalysisModel::fromMap(map);
}

bool DatabaseService::clearUserVehicleCache(int userId)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM cached_vehicles WHERE user_id = :uid");
    q.bindValue(":uid", userId);
    return execQuery(q);
}

void DatabaseService::clearCache()
{
    QSqlQuery q(m_db);
    q.exec("DELETE FROM cached_vehicles");
    q.exec("DELETE FROM cached_analysis");
}
