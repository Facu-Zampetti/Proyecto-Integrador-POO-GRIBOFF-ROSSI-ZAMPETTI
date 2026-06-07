#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QString>
#include "base/IManager.h"
#include "base/Singleton.h"

/**
 * @file ConfigManager.h
 * @brief Gestor de configuración de la aplicación. Singleton via template CRTP. Implementa IManager.
 *
 * Provee acceso centralizado a las configuraciones del sistema:
 *   - URL RTSP por defecto (configurable por el usuario).
 *   - Timeouts, parámetros de confianza, etc.
 *   - Persiste en SQLite via adminDB.
 */
class ConfigManager : public QObject, public IManager, public Singleton<ConfigManager> {
    Q_OBJECT

public:

    // ── IManager ──────────────────────────────────────────────────────
    void initialize() override;
    void shutdown()   override;
    bool isInitialized() const override { return m_initialized; }

    // ── Configuraciones ───────────────────────────────────────────────
    double  confidenceThreshold() const { return m_confidenceThreshold; }
    int     requestTimeoutMs()  const { return m_requestTimeoutMs; }
    bool    rememberSession()   const { return m_rememberSession; }
    QString rtspDefaultUrl()    const { return m_rtspDefaultUrl; }

    void setConfidenceThreshold(double threshold);
    void setRememberSession(bool remember);
    void setRtspDefaultUrl(const QString& url);

    static constexpr double      DEFAULT_CONFIDENCE  = 0.35;
    static constexpr int         DEFAULT_TIMEOUT_MS  = 30000;
    // URL RTSP por defecto: el media server (MediaMTX) del VPS Contabo, alimentado
    // por el telefono via Larix. Editable en runtime y persistida en SQLite.
    static constexpr const char* DEFAULT_RTSP_URL =
        "rtsp://carlensapp:CarlensRead2026mQ3@173.212.234.190:8554/live/carlens";

private:
    friend class Singleton<ConfigManager>; ///< Permite al template llamar al ctor privado.
    explicit ConfigManager(QObject* parent = nullptr);
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    void loadFromDatabase();
    void saveToDatabase();

    QString m_rtspDefaultUrl      = DEFAULT_RTSP_URL;
    double  m_confidenceThreshold = DEFAULT_CONFIDENCE;
    int     m_requestTimeoutMs    = DEFAULT_TIMEOUT_MS;
    bool    m_rememberSession     = false;
    bool    m_initialized         = false;
};

#endif // CONFIGMANAGER_H
