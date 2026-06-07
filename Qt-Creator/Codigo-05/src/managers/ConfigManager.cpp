#include "managers/ConfigManager.h"
#include "services/adminDB.h"
#include <QDebug>

ConfigManager::ConfigManager(QObject* parent)
    : QObject(parent)
{
}

void ConfigManager::initialize()
{
    if (m_initialized) return;
    loadFromDatabase();
    m_initialized = true;
    qInfo() << "[ConfigManager] RTSP por defecto:" << m_rtspDefaultUrl;
}

void ConfigManager::shutdown()
{
    saveToDatabase();
    m_initialized = false;
}

void ConfigManager::setConfidenceThreshold(double threshold)
{
    m_confidenceThreshold = threshold;
    saveToDatabase();
}

void ConfigManager::setRtspDefaultUrl(const QString& url)
{
    if (m_rtspDefaultUrl != url) {
        m_rtspDefaultUrl = url;
        saveToDatabase();
    }
}

void ConfigManager::setRememberSession(bool remember)
{
    m_rememberSession = remember;
    saveToDatabase();
}

void ConfigManager::loadFromDatabase()
{
    auto* db = adminDB::instance();
    m_rtspDefaultUrl      = db->loadSetting("rtsp_default_url", DEFAULT_RTSP_URL);
    m_confidenceThreshold = db->loadSetting("confidence",
                                            QString::number(DEFAULT_CONFIDENCE)).toDouble();
    m_requestTimeoutMs    = db->loadSetting("timeout_ms",
                                            QString::number(DEFAULT_TIMEOUT_MS)).toInt();
    m_rememberSession     = db->loadSetting("remember_session", "0") == "1";
}

void ConfigManager::saveToDatabase()
{
    auto* db = adminDB::instance();
    db->saveSetting("rtsp_default_url", m_rtspDefaultUrl);
    db->saveSetting("confidence",       QString::number(m_confidenceThreshold));
    db->saveSetting("timeout_ms",       QString::number(m_requestTimeoutMs));
    db->saveSetting("remember_session", m_rememberSession ? "1" : "0");
}
