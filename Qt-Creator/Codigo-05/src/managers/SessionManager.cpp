#include "managers/SessionManager.h"
#include "services/adminDB.h"
#include "models/UserModel.h"
#include <QDebug>

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
        adminDB::instance()->clearUserSession();
    m_initialized = false;
}

void SessionManager::login(const UserModel& user, bool rememberSession)
{
    m_currentUser  = user;
    m_rememberUser = rememberSession;

    if (rememberSession)
        adminDB::instance()->saveUserSession(user);

    emit sessionStarted(user);
    qInfo() << "[SessionManager] Sesión iniciada para:" << user;
}

void SessionManager::logout()
{
    adminDB::instance()->clearUserSession();
    m_currentUser  = UserModel{};
    m_rememberUser = false;
    emit sessionEnded();
    qInfo() << "[SessionManager] Sesión cerrada.";
}

bool SessionManager::tryRestoreSession()
{
    UserModel savedUser;
    if (adminDB::instance()->loadUserSession(savedUser) && savedUser.isValid()) {
        m_currentUser  = savedUser;
        m_rememberUser = true;
        emit sessionRestored(savedUser);
        qInfo() << "[SessionManager] Sesión restaurada para:" << savedUser;
        return true;
    }
    return false;
}
