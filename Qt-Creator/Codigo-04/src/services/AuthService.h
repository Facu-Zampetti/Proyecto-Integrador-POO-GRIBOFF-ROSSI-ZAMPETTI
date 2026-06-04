#ifndef AUTHSERVICE_H
#define AUTHSERVICE_H

#include "base/AbstractApiService.h"
#include "models/UserModel.h"
#include "services/RemoteAuthBackend.h"

/**
 * @file AuthService.h
 * @brief Servicio de autenticación. Hereda de AbstractApiService.
 *
 * Gestiona login y registro contra el backend REST.
 * Implementa las funciones virtuales puras de AbstractApiService.
 *
 * Principios aplicados:
 *   - Herencia: extiende AbstractApiService.
 *   - Polimorfismo: override de handleSuccess/handleError.
 *   - Encapsulamiento: estado interno de la operación actual.
 *   - Señales: loginSuccess, registerSuccess, etc.
 */
class AuthService : public AbstractApiService {
    Q_OBJECT

public:
    explicit AuthService(QObject* parent = nullptr);

    // ── Operaciones públicas ───────────────────────────────────────────
    void login(const QString& username, const QString& hashedPassword);
    void registerUser(const QString& username,
                      const QString& email,
                      const QString& hashedPassword);

    // ── Endpoints de la API ────────────────────────────────────────────
    static constexpr const char* EP_LOGIN    = "/api/auth/login";
    static constexpr const char* EP_REGISTER = "/api/auth/register";

    // Implementación de la interfaz abstracta (GET no es usado por AuthService)
    void get(const QString& endpoint) override;
    void post(const QString& endpoint, const QJsonObject& payload) override;

signals:
    void loginSuccess(const UserModel& user);
    void loginFailed(const QString& reason);
    void registerSuccess(const QString& message);
    void registerFailed(const QString& reason);

protected:
    void handleSuccess(const QString& endpoint,
                       const QJsonDocument& response) override;

    void handleError(const QString& endpoint,
                     const QString& errorMessage,
                     int httpStatusCode) override;

private:
    enum class PendingOp { None, Login, Register };
    PendingOp m_pendingOp = PendingOp::None;

    /// Camino real a la MySQL del VPS (helper Python + tunel SSH). Reemplaza al
    /// backend REST inexistente. Ver RemoteAuthBackend.
    RemoteAuthBackend m_remoteAuth;
};

#endif // AUTHSERVICE_H
