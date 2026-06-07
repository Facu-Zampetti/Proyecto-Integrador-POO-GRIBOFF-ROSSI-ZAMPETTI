#include "services/AuthService.h"
#include <QDebug>

AuthService::AuthService(QObject* parent)
    : QObject(parent)
{
}

// ─── Operaciones públicas ─────────────────────────────────────────────────

void AuthService::login(const QString& username, const QString& hashedPassword)
{
    // No existe backend REST: la unica via a la MySQL del VPS es el helper
    // Python por tunel SSH. Operacion sincrona (puede tardar unos segundos).
    const RemoteAuthBackend::Result res =
        m_remoteAuth.login(username, hashedPassword, QStringLiteral("127.0.0.1"),
                           QStringLiteral("CarlensDesktop"));

    if (!res.success) {
        emit loginFailed(res.error.isEmpty()
                             ? QStringLiteral("No se pudo iniciar sesion.")
                             : res.error);
        return;
    }

    const QString token = res.data.value(QStringLiteral("session_token")).toString();
    UserModel user(
        res.data.value(QStringLiteral("id")).toInt(),
        res.data.value(QStringLiteral("username")).toString(),
        res.data.value(QStringLiteral("email")).toString(),
        token
    );

    if (!user.isValid() || token.isEmpty()) {
        emit loginFailed(QStringLiteral("Respuesta de autenticacion invalida."));
        return;
    }

    emit loginSuccess(user);
}

void AuthService::registerUser(const QString& username,
                                const QString& email,
                                const QString& hashedPassword)
{
    const RemoteAuthBackend::Result res =
        m_remoteAuth.registerUser(username, email, hashedPassword);

    if (!res.success) {
        emit registerFailed(res.error.isEmpty()
                                ? QStringLiteral("No se pudo registrar el usuario.")
                                : res.error);
        return;
    }

    emit registerSuccess(QStringLiteral("Registro exitoso. Ya puede iniciar sesion."));
}
