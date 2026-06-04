#ifndef USERMANAGER_H
#define USERMANAGER_H

#include <QObject>
#include "base/IManager.h"
#include "models/UserModel.h"
#include "services/AuthService.h"

/**
 * @file UserManager.h
 * @brief Gestor de autenticación de usuarios. Implementa IManager.
 *
 * Orquesta el proceso de login y registro:
 *   1. Recibe las credenciales del widget de login.
 *   2. Hashea la contraseña con SHA-256.
 *   3. Delega al AuthService para la petición HTTP.
 *   4. Gestiona el resultado actualizando SessionManager.
 *   5. Propaga señales hacia los widgets.
 *
 * Principios: Mediador (patrón), IManager, composición con AuthService.
 */
class UserManager : public QObject, public IManager {
    Q_OBJECT

public:
    static UserManager* instance();

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
    explicit UserManager(QObject* parent = nullptr);
    ~UserManager() = default;
    UserManager(const UserManager&) = delete;
    UserManager& operator=(const UserManager&) = delete;

    AuthService* m_authService;
    bool         m_initialized  = false;
    bool         m_pendingRemember = false;

    static UserManager* s_instance;

private slots:
    void onLoginSuccess(const UserModel& user);
    void onLoginFailed(const QString& reason);
    void onRegisterSuccess(const QString& message);
    void onRegisterFailed(const QString& reason);
};

#endif // USERMANAGER_H
