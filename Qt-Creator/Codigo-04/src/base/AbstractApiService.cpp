#include "base/AbstractApiService.h"
#include "services/ApiClient.h"

AbstractApiService::AbstractApiService(QObject* parent)
    : QObject(parent)
    , m_apiClient(ApiClient::instance())
    , m_status(Carlens::ApiStatus::Idle)
{
    connect(m_apiClient, &ApiClient::responseReceived,
            this,        &AbstractApiService::onResponseReceived);

    connect(m_apiClient, &ApiClient::errorOccurred,
            this,        &AbstractApiService::onErrorOccurred);
}

Carlens::ApiStatus AbstractApiService::status() const
{
    return m_status;
}

void AbstractApiService::setStatus(Carlens::ApiStatus status)
{
    if (m_status != status) {
        m_status = status;
        emit statusChanged(m_status);
    }
}

void AbstractApiService::doGet(const QString& endpoint)
{
    setStatus(Carlens::ApiStatus::Loading);
    m_apiClient->get(endpoint);
}

void AbstractApiService::doPost(const QString& endpoint, const QJsonObject& payload)
{
    setStatus(Carlens::ApiStatus::Loading);
    m_apiClient->post(endpoint, payload);
}

void AbstractApiService::doPostMultipart(const QString& endpoint,
                                          const QString& filePath,
                                          const QVariantMap& fields)
{
    setStatus(Carlens::ApiStatus::Loading);
    m_apiClient->postMultipart(endpoint, filePath, fields);
}

// ─── Slots privados ───────────────────────────────────────────────────────

void AbstractApiService::onResponseReceived(const QString& endpoint,
                                             const QJsonDocument& doc)
{
    setStatus(Carlens::ApiStatus::Success);
    handleSuccess(endpoint, doc);
}

void AbstractApiService::onErrorOccurred(const QString& endpoint,
                                          const QString& error,
                                          int statusCode)
{
    setStatus(Carlens::ApiStatus::Error);
    handleError(endpoint, error, statusCode);
    emit requestFailed(error);
}
