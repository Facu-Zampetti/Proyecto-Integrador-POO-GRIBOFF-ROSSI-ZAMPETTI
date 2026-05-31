#include "managers/ConfigManager.h"
#include "services/DatabaseService.h"
#include "services/ApiClient.h"
#include <QDebug>

ConfigManager* ConfigManager::s_instance = nullptr;

ConfigManager* ConfigManager::instance()
{
    if (!s_instance)
        s_instance = new ConfigManager();
    return s_instance;
}

ConfigManager::ConfigManager(QObject* parent)
    : QObject(parent)
    , m_backendUrl(DEFAULT_BACKEND_URL)
{
}

void ConfigManager::initialize()
{
    if (m_initialized) return;
    loadFromDatabase();
    ApiClient::instance()->setBaseUrl(m_backendUrl);
    m_initialized = true;
    qInfo() << "[ConfigManager] Backend URL:" << m_backendUrl;
}

void ConfigManager::shutdown()
{
    saveToDatabase();
    m_initialized = false;
}

void ConfigManager::setBackendUrl(const QString& url)
{
    if (m_backendUrl != url) {
        m_backendUrl = url;
        ApiClient::instance()->setBaseUrl(url);
        saveToDatabase();
        emit backendUrlChanged(url);
    }
}

void ConfigManager::setConfidenceThreshold(double threshold)
{
    m_confidenceThreshold = threshold;
    saveToDatabase();
}

void ConfigManager::setRememberSession(bool remember)
{
    m_rememberSession = remember;
    saveToDatabase();
}

void ConfigManager::loadFromDatabase()
{
    auto* db = DatabaseService::instance();
    m_backendUrl          = db->loadSetting("backend_url",     DEFAULT_BACKEND_URL);
    m_confidenceThreshold = db->loadSetting("confidence",
                                            QString::number(DEFAULT_CONFIDENCE)).toDouble();
    m_requestTimeoutMs    = db->loadSetting("timeout_ms",
                                            QString::number(DEFAULT_TIMEOUT_MS)).toInt();
    m_rememberSession     = db->loadSetting("remember_session", "0") == "1";
}

void ConfigManager::saveToDatabase()
{
    auto* db = DatabaseService::instance();
    db->saveSetting("backend_url",      m_backendUrl);
    db->saveSetting("confidence",       QString::number(m_confidenceThreshold));
    db->saveSetting("timeout_ms",       QString::number(m_requestTimeoutMs));
    db->saveSetting("remember_session", m_rememberSession ? "1" : "0");
}
