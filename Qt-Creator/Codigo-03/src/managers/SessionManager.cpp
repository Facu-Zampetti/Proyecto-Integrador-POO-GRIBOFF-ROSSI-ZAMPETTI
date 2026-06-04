#include "managers/SessionManager.h"
#include "services/ApiClient.h"
#include "services/DatabaseService.h"
#include <QDebug>

SessionManager* SessionManager::s_instance = nullptr;

SessionManager* SessionManager::instance()
{
    if (!s_instance)
        s_instance = new SessionManager();
    return s_instance;
}

SessionManager::SessionManager(QObject* parent)
    : QObject(parent)
{
}

void SessionManager::initialize()
{
    if (m_initialized) return;
    m_initialized = true;
    qInfo() << "[SessionManager] Inicializado.";
}

void SessionManager::shutdown()
{
    if (!m_rememberUser)
        DatabaseService::instance()->clearUserSession();
    m_initialized = false;
}

void SessionManager::login(const UserModel& user, bool rememberSession)
{
    m_currentUser  = user;
    m_rememberUser = rememberSession;

    // Configurar el token JWT en el ApiClient global
    ApiClient::instance()->setAuthToken(user.token());

    if (rememberSession)
        DatabaseService::instance()->saveUserSession(user);

    emit sessionStarted(user);
    qInfo() << "[SessionManager] Sesión iniciada para:" << user.username();
}

void SessionManager::logout()
{
    ApiClient::instance()->abortAllRequests();
    DatabaseService::instance()->clearUserSession();
    ApiClient::instance()->clearAuthToken();
    m_currentUser  = UserModel{};
    m_rememberUser = false;
    emit sessionEnded();
    qInfo() << "[SessionManager] Sesión cerrada.";
}

bool SessionManager::tryRestoreSession()
{
    UserModel savedUser;
    if (DatabaseService::instance()->loadUserSession(savedUser) && savedUser.isValid()) {
        m_currentUser  = savedUser;
        m_rememberUser = true;
        ApiClient::instance()->setAuthToken(savedUser.token());
        emit sessionRestored(savedUser);
        qInfo() << "[SessionManager] Sesión restaurada para:" << savedUser.username();
        return true;
    }
    return false;
}
