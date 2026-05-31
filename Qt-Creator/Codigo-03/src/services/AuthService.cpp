#include "services/AuthService.h"
#include "services/ApiClient.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>

AuthService::AuthService(QObject* parent)
    : AbstractApiService(parent)
{
}

// ─── Operaciones públicas ─────────────────────────────────────────────────

void AuthService::login(const QString& username, const QString& hashedPassword)
{
    m_pendingOp = PendingOp::Login;

    QJsonObject payload;
    payload["username"] = username;
    payload["password"] = hashedPassword;

    doPost(EP_LOGIN, payload);
}

void AuthService::registerUser(const QString& username,
                                const QString& email,
                                const QString& hashedPassword)
{
    m_pendingOp = PendingOp::Register;

    QJsonObject payload;
    payload["username"] = username;
    payload["email"]    = email;
    payload["password"] = hashedPassword;

    doPost(EP_REGISTER, payload);
}

void AuthService::get(const QString& endpoint)
{
    doGet(endpoint);
}

void AuthService::post(const QString& endpoint, const QJsonObject& payload)
{
    doPost(endpoint, payload);
}

// ─── Manejo de respuestas (polimorfismo) ──────────────────────────────────

void AuthService::handleSuccess(const QString& endpoint,
                                 const QJsonDocument& response)
{
    QJsonObject obj = response.object();

    if (endpoint == EP_LOGIN) {
        // Respuesta esperada: { "access_token": "...", "user": {...} }
        QString token = obj.value("access_token").toString();
        QJsonObject userObj = obj.value("user").toObject();

        if (token.isEmpty() || userObj.isEmpty()) {
            emit loginFailed("Respuesta del servidor inválida.");
            return;
        }

        UserModel user(
            userObj.value("id").toInt(),
            userObj.value("username").toString(),
            userObj.value("email").toString(),
            token
        );

        emit loginSuccess(user);

    } else if (endpoint == EP_REGISTER) {
        // Respuesta esperada: { "message": "...", "user": {...} }
        QString msg = obj.value("message").toString("Registro exitoso.");
        emit registerSuccess(msg);
    }

    m_pendingOp = PendingOp::None;
}

void AuthService::handleError(const QString& endpoint,
                               const QString& errorMessage,
                               int httpStatusCode)
{
    QString finalMessage = errorMessage;

    if (httpStatusCode == 404) {
        finalMessage = QString(
            "Endpoint no encontrado (%1). Verifique la URL del backend (%2) y las rutas de autenticacion."
        ).arg(endpoint, ApiClient::instance()->baseUrl());
    }

    if (endpoint == EP_LOGIN) {
        emit loginFailed(finalMessage);
    } else if (endpoint == EP_REGISTER) {
        emit registerFailed(finalMessage);
    }

    m_pendingOp = PendingOp::None;
}
