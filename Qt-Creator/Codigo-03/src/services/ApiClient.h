#ifndef APICLIENT_H
#define APICLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QVariantMap>

/**
 * @file ApiClient.h
 * @brief Cliente HTTP centralizado. Singleton.
 *
 * Encapsula QNetworkAccessManager y expone una API uniforme para
 * todos los servicios REST del sistema. Gestiona:
 *   - Headers de autenticación JWT.
 *   - URL base configurable.
 *   - Peticiones GET, POST JSON y POST multipart/form-data.
 *   - Emisión de señales con la respuesta o el error.
 *
 * Principios aplicados:
 *   - Singleton: una sola instancia de QNetworkAccessManager.
 *   - Encapsulamiento: m_nam, m_baseUrl, m_authToken son privados.
 *   - Señales y slots: comunica resultado a servicios suscritos.
 */
class ApiClient : public QObject {
    Q_OBJECT

public:
    /// Retorna la instancia única del ApiClient.
    static ApiClient* instance();

    // ── Configuración ─────────────────────────────────────────────────
    void setBaseUrl(const QString& url);
    void setAuthToken(const QString& token);
    void clearAuthToken();
    void abortAllRequests();

    const QString& baseUrl()    const { return m_baseUrl; }
    const QString& authToken()  const { return m_authToken; }
    bool           hasToken()   const { return !m_authToken.isEmpty(); }

    // ── Peticiones HTTP ───────────────────────────────────────────────
    void get(const QString& endpoint);
    void post(const QString& endpoint, const QJsonObject& payload);

    void postMultipart(const QString& endpoint,
                       const QString& filePath,
                       const QVariantMap& extraFields = {});

signals:
    /// Emitido cuando una respuesta 2xx es recibida correctamente.
    void responseReceived(const QString& endpoint, const QJsonDocument& doc);

    /// Emitido ante error de red o código HTTP de error (4xx/5xx).
    void errorOccurred(const QString& endpoint,
                       const QString& errorMessage,
                       int httpStatusCode);

    /// Emitido con el progreso de una carga de archivo (0-100).
    void uploadProgress(const QString& endpoint, int percent);

private:
    explicit ApiClient(QObject* parent = nullptr);
    ~ApiClient() = default;
    ApiClient(const ApiClient&) = delete;
    ApiClient& operator=(const ApiClient&) = delete;

    QNetworkRequest buildRequest(const QString& endpoint) const;

    // ── Slots privados para manejar respuestas ─────────────────────────
    void handleReply(QNetworkReply* reply, const QString& endpoint);

    QNetworkAccessManager* m_nam;
    QString                m_baseUrl;
    QString                m_authToken;

    static ApiClient*      s_instance;
};

#endif // APICLIENT_H
