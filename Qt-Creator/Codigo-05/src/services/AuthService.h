#ifndef AUTHSERVICE_H
#define AUTHSERVICE_H

#include <QObject>

#include "models/UserModel.h"
#include "services/RemoteAuthBackend.h"

/**
 * @file AuthService.h
 * @brief Servicio de autenticación.
 *
 * Gestiona login y registro contra la MySQL del VPS. No existe un backend REST:
 * la única vía es el helper Python por túnel SSH encapsulado en RemoteAuthBackend.
 *
 * Principios aplicados:
 *   - Encapsulamiento: el detalle del túnel SSH queda en RemoteAuthBackend.
 *   - Señales: loginSuccess, registerSuccess, etc.
 */
class AuthService : public QObject {
    Q_OBJECT

public:
    explicit AuthService(QObject* parent = nullptr);

    // ── Operaciones públicas ───────────────────────────────────────────
    void login(const QString& username, const QString& hashedPassword);
    void registerUser(const QString& username,
                      const QString& email,
                      const QString& hashedPassword);

signals:
    void loginSuccess(const UserModel& user);
    void loginFailed(const QString& reason);
    void registerSuccess(const QString& message);
    void registerFailed(const QString& reason);

private:
    /// Camino real a la MySQL del VPS (helper Python + tunel SSH).
    RemoteAuthBackend m_remoteAuth;
};

#endif // AUTHSERVICE_H
