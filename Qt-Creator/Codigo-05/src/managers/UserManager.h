#ifndef USERMANAGER_H
#define USERMANAGER_H

#include <QObject>
#include "base/IManager.h"
#include "base/Singleton.h"
#include "models/UserModel.h"
#include "services/AuthService.h"

/**
 * @file UserManager.h
 * @brief Gestor de autenticación de usuarios. Implementa IManager. Singleton via template CRTP.
 *
 * Orquesta el proceso de login y registro:
 *   1. Recibe las credenciales del widget de login.
 *   2. Hashea la contraseña con SHA-256.
 *   3. Delega al AuthService para comunicación remota.
 *   4. Gestiona el resultado actualizando SessionManager.
 *   5. Propaga señales hacia los widgets.
 *
 * Hereda de Singleton<UserManager> en lugar de repetir el boilerplate
 * (s_instance + instance()). Ver src/base/Singleton.h.
 *
 * Principios: Mediador (patrón), IManager, template CRTP Singleton.
 */
class UserManager : public QObject, public IManager, public Singleton<UserManager> {
    Q_OBJECT

public:

    // ── IManager ──────────────────────────────────────────────────────
    void initialize() override;
    void shutdown()   override;
    bool isInitialized() const override { return m_initialized; }

    // ── Operaciones ───────────────────────────────────────────────────
    void login(const QString& username,
               const QString& plainPassword,
               bool           rememberSession = false);

    void registerUser(const QString& username,
                      const QString& email,
                      const QString& plainPassword);

    void logout();

    bool isLoggedIn() const;

signals:
    void loginSuccess(const UserModel& user);
    void loginFailed(const QString& reason);
    void registerSuccess(const QString& message);
    void registerFailed(const QString& reason);
    void logoutCompleted();

private:
    friend class Singleton<UserManager>; ///< Permite al template llamar al ctor privado.
    explicit UserManager(QObject* parent = nullptr);
    ~UserManager() = default;
    UserManager(const UserManager&) = delete;
    UserManager& operator=(const UserManager&) = delete;

    AuthService* m_authService;
    bool         m_initialized    = false;
    bool         m_pendingRemember = false;

private slots:
    void onLoginSuccess(const UserModel& user);
    void onLoginFailed(const QString& reason);
    void onRegisterSuccess(const QString& message);
    void onRegisterFailed(const QString& reason);
};

#endif // USERMANAGER_H
