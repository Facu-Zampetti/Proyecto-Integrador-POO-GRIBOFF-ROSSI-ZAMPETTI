#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QObject>
#include "base/IManager.h"
#include "models/UserModel.h"

/**
 * @file SessionManager.h
 * @brief Gestor de sesión del usuario autenticado. Singleton. Implementa IManager.
 *
 * Responsabilidades:
 *   - Mantener el usuario actualmente logueado en memoria.
 *   - Guardar/cargar sesión persistida (recordar sesión) via DatabaseService.
 *   - Configurar el token JWT en ApiClient tras un login exitoso.
 *   - Notificar cambios de sesión al resto de la aplicación.
 *
 * Principios aplicados: Singleton, IManager, Encapsulamiento, Señales.
 */
class SessionManager : public QObject, public IManager {
    Q_OBJECT

public:
    static SessionManager* instance();

    // ── IManager ──────────────────────────────────────────────────────
    void initialize() override;
    void shutdown()   override;
    bool isInitialized() const override { return m_initialized; }

    // ── Estado de sesión ──────────────────────────────────────────────
    bool            isLoggedIn()   const { return m_currentUser.isValid(); }
    const UserModel& currentUser() const { return m_currentUser; }
    const QString&  authToken()    const { return m_currentUser.token(); }

    // ── Acciones ──────────────────────────────────────────────────────
    void login(const UserModel& user, bool rememberSession = false);
    void logout();

    /// Intenta restaurar una sesión guardada. Emite sessionRestored o no.
    bool tryRestoreSession();

signals:
    void sessionStarted(const UserModel& user);
    void sessionEnded();
    void sessionRestored(const UserModel& user);

private:
    explicit SessionManager(QObject* parent = nullptr);
    ~SessionManager() = default;
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

    UserModel m_currentUser;
    bool      m_initialized  = false;
    bool      m_rememberUser = false;

    static SessionManager* s_instance;
};

#endif // SESSIONMANAGER_H
