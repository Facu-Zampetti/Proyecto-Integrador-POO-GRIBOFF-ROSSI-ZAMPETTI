#ifndef ABSTRACTAPISERVICE_H
#define ABSTRACTAPISERVICE_H

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

#include "utils/AppEnums.h"

// Forward declaration para evitar inclusión circular
class ApiClient;

/**
 * @file AbstractApiService.h
 * @brief Clase abstracta base para todos los servicios de la API REST.
 *
 * Define el contrato de comunicación con el backend:
 *   - Expone métodos get/post/postMultipart para subclases.
 *   - Declara handleSuccess y handleError como funciones virtuales puras.
 *   - Gestiona el estado (ApiStatus) de la petición.
 *
 * Principios aplicados:
 *   - Clase abstracta con funciones virtuales puras.
 *   - Herencia: AuthService, VideoService, AnalysisService la extienden.
 *   - Encapsulamiento: m_apiClient y m_status son protegidos.
 *   - Señales y slots: emite requestFailed ante errores de red.
 */
class AbstractApiService : public QObject {
    Q_OBJECT

public:
    explicit AbstractApiService(QObject* parent = nullptr);
    virtual ~AbstractApiService() = default;

    // Contrato publico comun para operaciones HTTP basicas.
    virtual void get(const QString& endpoint) = 0;
    virtual void post(const QString& endpoint, const QJsonObject& payload) = 0;

    /// Retorna el estado actual de la petición.
    Carlens::ApiStatus status() const;

protected:
    // ── Métodos de conveniencia para subclases ─────────────────────────
    void doGet(const QString& endpoint);
    void doPost(const QString& endpoint, const QJsonObject& payload);
    void doPostMultipart(const QString& endpoint,
                         const QString& filePath,
                         const QVariantMap& fields = {});

    // ── Contrato que TODA subclase debe implementar ────────────────────
    /// Llamado cuando la petición HTTP finaliza con éxito (2xx).
    virtual void handleSuccess(const QString& endpoint,
                               const QJsonDocument& response) = 0;

    /// Llamado cuando la petición HTTP falla (4xx/5xx o error de red).
    virtual void handleError(const QString& endpoint,
                             const QString& errorMessage,
                             int httpStatusCode) = 0;

    void setStatus(Carlens::ApiStatus status);

    ApiClient*          m_apiClient;   ///< Puntero al singleton ApiClient (no owned)
    Carlens::ApiStatus  m_status;

signals:
    /// Emitido cuando el status interno cambia.
    void statusChanged(Carlens::ApiStatus newStatus);

    /// Emitido ante cualquier error de red o HTTP.
    void requestFailed(const QString& errorMessage);

private slots:
    void onResponseReceived(const QString& endpoint, const QJsonDocument& doc);
    void onErrorOccurred(const QString& endpoint,
                         const QString& error,
                         int statusCode);
};

#endif // ABSTRACTAPISERVICE_H
