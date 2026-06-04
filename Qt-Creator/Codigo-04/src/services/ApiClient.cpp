#include "services/ApiClient.h"

#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QMimeDatabase>
#include <QMimeType>
#include <QJsonParseError>
#include <QDebug>

namespace {
constexpr int REQUEST_TIMEOUT_MS = 20 * 60 * 1000;
}

ApiClient* ApiClient::s_instance = nullptr;

ApiClient* ApiClient::instance()
{
    if (!s_instance) {
        s_instance = new ApiClient();
    }
    return s_instance;
}

ApiClient::ApiClient(QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_baseUrl("http://127.0.0.1:8000")
{
}

// ─── Configuración ────────────────────────────────────────────────────────

void ApiClient::setBaseUrl(const QString& url)
{
    m_baseUrl = url;
    // Eliminar barra final si existe
    if (m_baseUrl.endsWith('/'))
        m_baseUrl.chop(1);
}

void ApiClient::setAuthToken(const QString& token)
{
    m_authToken = token;
}

void ApiClient::clearAuthToken()
{
    m_authToken.clear();
}

void ApiClient::abortAllRequests()
{
    const auto replies = m_nam->findChildren<QNetworkReply*>();
    for (QNetworkReply* reply : replies) {
        if (reply && reply->isRunning())
            reply->abort();
    }
}

// ─── Construcción de request ──────────────────────────────────────────────

QNetworkRequest ApiClient::buildRequest(const QString& endpoint) const
{
    QString url = m_baseUrl;
    if (!endpoint.startsWith('/'))
        url += '/';
    url += endpoint;

    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(REQUEST_TIMEOUT_MS);

    if (!m_authToken.isEmpty())
        request.setRawHeader("Authorization",
                             QByteArray("Bearer ") + m_authToken.toUtf8());

    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    return request;
}

// ─── GET ──────────────────────────────────────────────────────────────────

void ApiClient::get(const QString& endpoint)
{
    QNetworkReply* reply = m_nam->get(buildRequest(endpoint));
    connect(reply, &QNetworkReply::finished, this, [this, reply, endpoint]() {
        handleReply(reply, endpoint);
    });
}

// ─── POST JSON ────────────────────────────────────────────────────────────

void ApiClient::post(const QString& endpoint, const QJsonObject& payload)
{
    QNetworkRequest req = buildRequest(endpoint);
    QByteArray body     = QJsonDocument(payload).toJson(QJsonDocument::Compact);

    QNetworkReply* reply = m_nam->post(req, body);
    connect(reply, &QNetworkReply::finished, this, [this, reply, endpoint]() {
        handleReply(reply, endpoint);
    });
}

// ─── POST Multipart ───────────────────────────────────────────────────────

void ApiClient::postMultipart(const QString& endpoint,
                               const QString& filePath,
                               const QVariantMap& extraFields)
{
    QFile* file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        delete file;
        emit errorOccurred(endpoint, "No se pudo abrir el archivo: " + filePath, 0);
        return;
    }

    QMimeDatabase   mimeDb;
    QMimeType       mimeType = mimeDb.mimeTypeForFile(filePath);
    QFileInfo       fi(filePath);

    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // ── Parte del archivo ──────────────────────────────────────────────
    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader,
                       QVariant(mimeType.name()));
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant(QString("form-data; name=\"file\"; filename=\"%1\"")
                                .arg(fi.fileName())));
    file->setParent(multiPart);
    filePart.setBodyDevice(file);
    multiPart->append(filePart);

    // ── Campos extra ──────────────────────────────────────────────────
    for (auto it = extraFields.cbegin(); it != extraFields.cend(); ++it) {
        QHttpPart textPart;
        textPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                           QVariant(QString("form-data; name=\"%1\"").arg(it.key())));
        textPart.setBody(it.value().toString().toUtf8());
        multiPart->append(textPart);
    }

    // ── Request sin Content-Type (lo setea Qt automáticamente para multipart) ─
    QString url = m_baseUrl;
    if (!endpoint.startsWith('/')) url += '/';
    url += endpoint;

    QNetworkRequest req{QUrl(url)};
    req.setTransferTimeout(REQUEST_TIMEOUT_MS);
    if (!m_authToken.isEmpty())
        req.setRawHeader("Authorization",
                         QByteArray("Bearer ") + m_authToken.toUtf8());

    QNetworkReply* reply = m_nam->post(req, multiPart);
    multiPart->setParent(reply);

    connect(reply, &QNetworkReply::uploadProgress, this,
            [this, endpoint](qint64 sent, qint64 total) {
                if (total > 0)
                    emit uploadProgress(endpoint,
                                        static_cast<int>((sent * 100) / total));
            });

    connect(reply, &QNetworkReply::finished, this, [this, reply, endpoint]() {
        handleReply(reply, endpoint);
    });
}

// ─── Manejo unificado de respuestas ───────────────────────────────────────

void ApiClient::handleReply(QNetworkReply* reply, const QString& endpoint)
{
    reply->deleteLater();

    int statusCode = reply->attribute(
                         QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() != QNetworkReply::NoError) {
        QString errMsg = reply->errorString();
        // Intentar leer el body del error para más contexto
        QByteArray body = reply->readAll();
        if (!body.isEmpty()) {
            QJsonParseError parseErr;
            QJsonDocument doc = QJsonDocument::fromJson(body, &parseErr);
            if (parseErr.error == QJsonParseError::NoError && doc.isObject()) {
                QString detail = doc.object().value("detail").toString();
                if (!detail.isEmpty())
                    errMsg = detail;
            }
        }
        qWarning() << "[ApiClient] Error en" << endpoint
                   << "HTTP" << statusCode << ":" << errMsg;
        emit errorOccurred(endpoint, errMsg, statusCode);
        return;
    }

    QByteArray      rawData = reply->readAll();
    QJsonParseError parseErr;
    QJsonDocument   doc     = QJsonDocument::fromJson(rawData, &parseErr);

    if (parseErr.error != QJsonParseError::NoError) {
        emit errorOccurred(endpoint,
                           "Respuesta JSON inválida: " + parseErr.errorString(),
                           statusCode);
        return;
    }

    emit responseReceived(endpoint, doc);
}
