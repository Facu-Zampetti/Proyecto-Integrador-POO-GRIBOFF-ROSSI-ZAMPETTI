#ifndef REMOTEAUTHBACKEND_H
#define REMOTEAUTHBACKEND_H

#include <QJsonObject>
#include <QString>

/**
 * @file RemoteAuthBackend.h
 * @brief Puente C++ -> helper Python (backend/remote_auth_sync.py) para
 *        registrar/iniciar sesion de usuarios en la MySQL del VPS via tunel SSH.
 *
 * Espejo de RemoteSnapshotPersistenceBackend::executeHelper: lanza el helper por
 * QProcess con --config y --payload (JSON temporal) y parsea la respuesta JSON.
 * No existe backend REST; este es el unico camino funcional a la base remota.
 *
 * NOTA: las operaciones son sincronas (waitForFinished). El llamador es
 * responsable de invocarlas fuera del hilo de UI si quiere evitar congelar la
 * ventana mientras se abre el tunel SSH.
 */
class RemoteAuthBackend {
public:
    struct Result {
        bool        success = false;
        QString     error;       ///< Mensaje de error listo para mostrar al usuario.
        QJsonObject data;        ///< Objeto "data" devuelto por el helper en exito.
    };

    RemoteAuthBackend() = default;

    /// INSERT en users. data: { id, username, email, role, is_active }.
    Result registerUser(const QString& username,
                        const QString& email,
                        const QString& passwordHash);

    /// Verifica credenciales + crea user_sessions. data: { id, username, email,
    /// role, session_token, expires_at }.
    Result login(const QString& username,
                 const QString& passwordHash,
                 const QString& ipAddress = QString(),
                 const QString& userAgent = QString());

private:
    Result executeHelper(const QString& command, const QJsonObject& payload);

    /// Sube directorios desde el dir del ejecutable buscando el que contiene
    /// backend/remote_auth_sync.py (directamente o en Codigo_Implementacion_IA/).
    QString findProjectRoot() const;

    QString resolvePythonExe(const QString& projectRoot) const;
    QString resolveHelperPath(const QString& projectRoot) const;
    QString resolveConfigPath(const QString& projectRoot) const;
};

#endif // REMOTEAUTHBACKEND_H
