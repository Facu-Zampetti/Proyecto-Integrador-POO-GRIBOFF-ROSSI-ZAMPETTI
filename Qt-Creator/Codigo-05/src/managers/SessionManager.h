#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QObject>
#include "base/IManager.h"
#include "base/Singleton.h"
#include "models/UserModel.h"

/**
 * @file SessionManager.h
 * @brief Gestor de sesión del usuario autenticado. Singleton via template CRTP. Implementa IManager.
 *
 * Responsabilidades:
 *   - Mantener el usuario actualmente logueado en memoria.
 *   - Guardar/cargar sesión persistida (recordar sesión) via adminDB.
 *   - Notificar cambios de sesión al resto de la aplicación.
 *
 * Principios aplicados: Singleton template CRTP, IManager, Encapsulamiento, Señales.
 */
class SessionManager : public QObject, public IManager, public Singleton<SessionManager> {
    Q_OBJECT

public:

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
    friend class Singleton<SessionManager>; ///< Permite al template llamar al ctor privado.
    explicit SessionManager(QObject* parent = nullptr);
    ~SessionManager() = default;
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

    UserModel m_currentUser;
    bool      m_initialized  = false;
    bool      m_rememberUser = false;
};

#endif // SESSIONMANAGER_H
