#include "managers/UserManager.h"
#include "managers/SessionManager.h"
#include "utils/CryptoHelper.h"
#include <QDebug>

UserManager* UserManager::s_instance = nullptr;

UserManager* UserManager::instance()
{
    if (!s_instance)
        s_instance = new UserManager();
    return s_instance;
}

UserManager::UserManager(QObject* parent)
    : QObject(parent)
    , m_authService(new AuthService(this))
{
}

void UserManager::initialize()
{
    if (m_initialized) return;

    connect(m_authService, &AuthService::loginSuccess,
            this,          &UserManager::onLoginSuccess);
    connect(m_authService, &AuthService::loginFailed,
            this,          &UserManager::onLoginFailed);
    connect(m_authService, &AuthService::registerSuccess,
            this,          &UserManager::onRegisterSuccess);
    connect(m_authService, &AuthService::registerFailed,
            this,          &UserManager::onRegisterFailed);

    m_initialized = true;
}

void UserManager::shutdown()
{
    m_initialized = false;
}

// ─── Operaciones ──────────────────────────────────────────────────────────

void UserManager::login(const QString& username,
                         const QString& plainPassword,
                         bool           rememberSession)
{
    m_pendingRemember = rememberSession;
    QString hashed    = CryptoHelper::sha256(plainPassword);
    m_authService->login(username, hashed);
}

void UserManager::registerUser(const QString& username,
                                const QString& email,
                                const QString& plainPassword)
{
    QString hashed = CryptoHelper::sha256(plainPassword);
    m_authService->registerUser(username, email, hashed);
}

void UserManager::logout()
{
    SessionManager::instance()->logout();
    emit logoutCompleted();
}

bool UserManager::isLoggedIn() const
{
    return SessionManager::instance()->isLoggedIn();
}

// ─── Slots privados ───────────────────────────────────────────────────────

void UserManager::onLoginSuccess(const UserModel& user)
{
    SessionManager::instance()->login(user, m_pendingRemember);
    emit loginSuccess(user);
}

void UserManager::onLoginFailed(const QString& reason)
{
    emit loginFailed(reason);
}

void UserManager::onRegisterSuccess(const QString& message)
{
    emit registerSuccess(message);
}

void UserManager::onRegisterFailed(const QString& reason)
{
    emit registerFailed(reason);
}
