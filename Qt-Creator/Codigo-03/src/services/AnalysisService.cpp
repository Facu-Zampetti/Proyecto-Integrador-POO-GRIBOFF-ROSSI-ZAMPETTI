#include "services/AnalysisService.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

AnalysisService::AnalysisService(QObject* parent)
    : AbstractApiService(parent)
{
}

void AnalysisService::get(const QString& endpoint)
{
    doGet(endpoint);
}

void AnalysisService::post(const QString& endpoint, const QJsonObject& payload)
{
    doPost(endpoint, payload);
}

// ─── Operaciones públicas ─────────────────────────────────────────────────

void AnalysisService::requestAnalysis(int vehicleId)
{
    doPost(QString("%1/%2/request").arg(EP_ANALYSIS_BASE).arg(vehicleId), {});
}

void AnalysisService::getHistory(int page, int perPage)
{
    doGet(QString("%1?page=%2&per_page=%3").arg(EP_HISTORY).arg(page).arg(perPage));
}

void AnalysisService::getVehicleDetails(int vehicleId)
{
    doGet(QString("%1/%2").arg(EP_HISTORY).arg(vehicleId));
}

void AnalysisService::getAnalysis(int vehicleId)
{
    doGet(QString("%1/%2").arg(EP_ANALYSIS_BASE).arg(vehicleId));
}

// ─── Manejo de respuestas ─────────────────────────────────────────────────

void AnalysisService::handleSuccess(const QString& endpoint,
                                     const QJsonDocument& response)
{
    QJsonObject obj = response.object();

    // Detección por contenido del endpoint
    if (endpoint.contains("/request")) {
        // Análisis recién generado
        AnalysisModel analysis = parseAnalysis(obj.value("analysis").toObject());
        emit analysisReceived(analysis);

    } else if (endpoint.startsWith(EP_ANALYSIS_BASE) &&
               !endpoint.contains("/request")) {
        // GET de análisis existente
        AnalysisModel analysis = parseAnalysis(obj);
        emit analysisReceived(analysis);

    } else if (endpoint.startsWith(EP_HISTORY)) {
        if (obj.contains("vehicles")) {
            // Listado de historial
            QList<VehicleModel> vehicles;
            QJsonArray arr = obj.value("vehicles").toArray();
            for (const QJsonValue& v : arr)
                vehicles.append(parseVehicle(v.toObject()));

            int total = obj.value("total").toInt(vehicles.size());
            emit historyReceived(vehicles, total);

        } else {
            // Detalle de un vehículo
            VehicleModel  vehicle  = parseVehicle(obj.value("vehicle").toObject());
            AnalysisModel analysis = parseAnalysis(obj.value("analysis").toObject());
            emit vehicleDetailsReceived(vehicle, analysis);
        }
    }
}

void AnalysisService::handleError(const QString& endpoint,
                                   const QString& errorMessage,
                                   int httpStatusCode)
{
    Q_UNUSED(httpStatusCode)

    if (endpoint.contains("/request") ||
        endpoint.startsWith(EP_ANALYSIS_BASE)) {
        emit analysisFailed(errorMessage);
    } else if (endpoint.startsWith(EP_HISTORY)) {
        if (endpoint == EP_HISTORY || endpoint.startsWith(EP_HISTORY + QString("?")))
            emit historyFailed(errorMessage);
        else
            emit vehicleDetailsFailed(errorMessage);
    }
}

// ─── Helpers ──────────────────────────────────────────────────────────────

VehicleModel AnalysisService::parseVehicle(const QJsonObject& obj) const
{
    QVariantMap map;
    map["id"]            = obj.value("id").toInt();
    map["session_id"]    = obj.value("session_id").toInt();
    map["snapshot_path"] = obj.value("snapshot_path").toString();
    map["snapshot_url"]  = obj.value("snapshot_url").toString();
    // Para el enum, almacenamos el int para compatibilidad con QVariantMap
    map["vehicle_type"]  = static_cast<int>(
                               VehicleModel::vehicleTypeFromString(
                                   obj.value("vehicle_type").toString()));
    map["brand"]         = obj.value("brand").toString();
    map["model"]         = obj.value("model").toString();
    map["year_range"]    = obj.value("year_range").toString();
    map["color"]         = obj.value("color").toString();
    map["license_plate"] = obj.value("license_plate").toString();
    map["confidence"]    = obj.value("confidence").toDouble();
    map["detected_at"]   = obj.value("detected_at").toString();
    map["has_analysis"]  = obj.value("has_analysis").toBool() ? 1 : 0;
    return VehicleModel::fromMap(map);
}

AnalysisModel AnalysisService::parseAnalysis(const QJsonObject& obj) const
{
    if (obj.isEmpty()) return AnalysisModel{};

    QVariantMap map;
    map["id"]           = obj.value("id").toInt();
    map["vehicle_id"]   = obj.value("vehicle_id").toInt();
    map["brand"]        = obj.value("brand").toString();
    map["model"]        = obj.value("model").toString();
    map["color"]        = obj.value("color").toString();
    map["vehicle_type"] = obj.value("vehicle_type").toString();
    map["license_plate"]= obj.value("license_plate").toString();
    map["observations"] = obj.value("observations").toString();
    map["full_text"]    = obj.value("full_text").toString();
    map["analyzed_at"]  = obj.value("analyzed_at").toString();
    return AnalysisModel::fromMap(map);
}
