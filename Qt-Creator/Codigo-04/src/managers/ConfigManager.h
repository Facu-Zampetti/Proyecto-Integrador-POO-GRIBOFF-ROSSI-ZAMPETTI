#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QString>
#include "base/IManager.h"

/**
 * @file ConfigManager.h
 * @brief Gestor de configuración de la aplicación. Singleton. Implementa IManager.
 *
 * Provee acceso centralizado a las configuraciones del sistema:
 *   - URL del backend (configurable por el usuario).
 *   - Timeouts, parámetros de confianza, etc.
 *   - Persiste en SQLite via DatabaseService.
 */
class ConfigManager : public QObject, public IManager {
    Q_OBJECT

public:
    static ConfigManager* instance();

    // ── IManager ──────────────────────────────────────────────────────
    void initialize() override;
    void shutdown()   override;
    bool isInitialized() const override { return m_initialized; }

    // ── Configuraciones ───────────────────────────────────────────────
    QString backendUrl()        const { return m_backendUrl; }
    double  confidenceThreshold() const { return m_confidenceThreshold; }
    int     requestTimeoutMs()  const { return m_requestTimeoutMs; }
    bool    rememberSession()   const { return m_rememberSession; }

    void setBackendUrl(const QString& url);
    void setConfidenceThreshold(double threshold);
    void setRememberSession(bool remember);

    static constexpr const char* DEFAULT_BACKEND_URL = "http://127.0.0.1:8000";
    static constexpr double      DEFAULT_CONFIDENCE  = 0.35;
    static constexpr int         DEFAULT_TIMEOUT_MS  = 30000;

signals:
    void backendUrlChanged(const QString& newUrl);

private:
    explicit ConfigManager(QObject* parent = nullptr);
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    void loadFromDatabase();
    void saveToDatabase();

    QString m_backendUrl;
    double  m_confidenceThreshold = DEFAULT_CONFIDENCE;
    int     m_requestTimeoutMs    = DEFAULT_TIMEOUT_MS;
    bool    m_rememberSession     = false;
    bool    m_initialized         = false;

    static ConfigManager* s_instance;
};

#endif // CONFIGMANAGER_H
