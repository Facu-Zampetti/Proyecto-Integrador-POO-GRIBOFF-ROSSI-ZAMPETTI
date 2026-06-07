#include "services/RemoteHistoryBackend.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QProcess>
#include <QTemporaryFile>
#include <QUrl>
#include <QVariantMap>

namespace {
constexpr const char* kHelperRelPath = "backend/remote_snapshot_sync.py";
constexpr const char* kConfigRelPath = "config/vehicle_surveillance.remote.json";
constexpr const char* kIaSubdir      = "Codigo_Implementacion_IA";

// Devuelve la raiz que contiene backend/remote_snapshot_sync.py a partir de un
// dir base: el propio base, o base/Codigo_Implementacion_IA. Vacio si nada.
QString matchHistoryRoot(const QDir& base)
{
    if (QFileInfo::exists(base.filePath(QString::fromLatin1(kHelperRelPath))))
        return base.absolutePath();

    const QString nested = base.filePath(
        QString::fromLatin1(kIaSubdir) + QLatin1Char('/') + QString::fromLatin1(kHelperRelPath));
    if (QFileInfo::exists(nested))
        return QDir(base.filePath(QString::fromLatin1(kIaSubdir))).absolutePath();

    return QString();
}

// Si existe la imagen local de la captura (carpeta captures/), devuelve su URL
// file:// para mostrarla en las tarjetas/detalle. Vacio si no esta localmente.
QString localSnapshotUrl(const QString& snapshotPath, const QString& capturesDir)
{
    if (snapshotPath.isEmpty() || capturesDir.isEmpty())
        return QString();
    const QString base = QFileInfo(snapshotPath).fileName();
    if (base.isEmpty())
        return QString();
    const QString local = QDir(capturesDir).filePath(base);
    if (QFileInfo::exists(local))
        return QUrl::fromLocalFile(local).toString();
    return QString();
}

VehicleModel parseVehicle(const QJsonObject& obj, const QString& capturesDir)
{
    const QString snapshotPath = obj.value("snapshot_path").toString();

    QVariantMap map;
    map["id"]            = obj.value("id").toInt();
    map["session_id"]    = obj.value("session_id").toInt();
    map["snapshot_path"] = snapshotPath;
    map["snapshot_url"]  = localSnapshotUrl(snapshotPath, capturesDir);
    map["vehicle_type"]  = static_cast<int>(
        VehicleModel::vehicleTypeFromString(obj.value("vehicle_type").toString()));
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

AnalysisModel parseAnalysis(const QJsonObject& obj)
{
    QVariantMap map;
    map["id"]            = obj.value("id").toInt();
    map["vehicle_id"]    = obj.value("vehicle_id").toInt();
    map["brand"]         = obj.value("brand").toString();
    map["model"]         = obj.value("model").toString();
    map["color"]         = obj.value("color").toString();
    map["vehicle_type"]  = obj.value("vehicle_type").toString();
    map["license_plate"] = obj.value("license_plate").toString();
    map["observations"]  = obj.value("observations").toString();
    map["full_text"]     = obj.value("full_text").toString();
    map["analyzed_at"]   = obj.value("analyzed_at").toString();
    return AnalysisModel::fromMap(map);
}
}

bool RemoteHistoryBackend::runHelper(const QString& command, const QJsonObject& payload,
                                     QJsonObject* dataOut, QString* errorOut,
                                     QString* projectRootOut)
{
    const QString projectRoot = findProjectRoot();
    if (projectRootOut)
        *projectRootOut = projectRoot;

    const QString pythonExe  = resolvePythonExe(projectRoot);
    const QString helperPath = resolveHelperPath(projectRoot);
    const QString configPath = resolveConfigPath(projectRoot);

    if (helperPath.isEmpty()) {
        if (errorOut) *errorOut = QStringLiteral("No se encontró el helper (remote_snapshot_sync.py).");
        return false;
    }
    if (!QFileInfo::exists(configPath)) {
        if (errorOut) *errorOut = QStringLiteral("Falta config/vehicle_surveillance.remote.json.");
        return false;
    }

    QTemporaryFile payloadFile;
    payloadFile.setAutoRemove(true);
    if (!payloadFile.open()) {
        if (errorOut) *errorOut = QStringLiteral("No se pudo crear un archivo temporal.");
        return false;
    }
    payloadFile.write(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    payloadFile.flush();

    QProcess process;
    process.setProgram(pythonExe);
    process.setArguments({helperPath, command,
                          QStringLiteral("--config"), configPath,
                          QStringLiteral("--payload"), payloadFile.fileName()});
    process.start();

    if (!process.waitForStarted(5000)) {
        if (errorOut) *errorOut = QStringLiteral("No se pudo iniciar el entorno Python.");
        return false;
    }
    if (!process.waitForFinished(45000)) {
        process.kill();
        process.waitForFinished(2000);
        if (errorOut) *errorOut = QStringLiteral("El servidor no respondió a tiempo.");
        return false;
    }

    const QByteArray stdOut = process.readAllStandardOutput().trimmed();
    const QByteArray stdErr = process.readAllStandardError().trimmed();
    const QByteArray jsonBytes = !stdOut.isEmpty() ? stdOut : stdErr;

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorOut)
            *errorOut = stdErr.isEmpty() ? QStringLiteral("Respuesta del servidor inválida.")
                                         : QString::fromUtf8(stdErr);
        return false;
    }

    const QJsonObject obj = doc.object();
    if (!obj.value(QStringLiteral("success")).toBool()) {
        if (errorOut)
            *errorOut = obj.value(QStringLiteral("error")).toString(
                QStringLiteral("La operación falló."));
        return false;
    }

    if (dataOut)
        *dataOut = obj.value(QStringLiteral("data")).toObject();
    return true;
}

RemoteHistoryBackend::Result RemoteHistoryBackend::listHistory(int userId, int page, int perPage)
{
    Result result;

    QJsonObject payload;
    if (userId > 0)
        payload.insert("user_id", userId);
    payload.insert("page", page > 0 ? page : 1);
    payload.insert("per_page", perPage > 0 ? perPage : 20);

    QJsonObject data;
    QString projectRoot;
    if (!runHelper(QStringLiteral("list-history"), payload, &data, &result.error, &projectRoot))
        return result;

    const QString capturesDir = QDir(projectRoot).filePath(QStringLiteral("captures"));
    const QJsonArray vehicles = data.value(QStringLiteral("vehicles")).toArray();
    for (const QJsonValue& value : vehicles)
        result.vehicles.append(parseVehicle(value.toObject(), capturesDir));

    result.total = data.value(QStringLiteral("total")).toInt(result.vehicles.size());
    result.success = true;
    return result;
}

RemoteHistoryBackend::AnalysisDetail RemoteHistoryBackend::getAnalysis(int vehicleId)
{
    AnalysisDetail detail;

    QJsonObject payload;
    payload.insert("vehicle_id", vehicleId);

    QJsonObject data;
    QString projectRoot;
    if (!runHelper(QStringLiteral("get-analysis"), payload, &data, &detail.error, &projectRoot))
        return detail;

    const QString capturesDir = QDir(projectRoot).filePath(QStringLiteral("captures"));
    detail.vehicle  = parseVehicle(data.value(QStringLiteral("vehicle")).toObject(), capturesDir);
    detail.analysis = parseAnalysis(data.value(QStringLiteral("analysis")).toObject());
    detail.success = true;
    return detail;
}

QString RemoteHistoryBackend::findProjectRoot() const
{
    QDir dir(QCoreApplication::applicationDirPath());

    for (int i = 0; i < 8; ++i) {
        const QString direct = matchHistoryRoot(dir);
        if (!direct.isEmpty())
            return direct;

        const QFileInfoList children = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo& child : children) {
            const QString fromChild = matchHistoryRoot(QDir(child.absoluteFilePath()));
            if (!fromChild.isEmpty())
                return fromChild;
        }

        if (!dir.cdUp())
            break;
    }

    static const char* const kAbsoluteCandidates[] = {
        "C:/Carlens/Codigo-03/Codigo_Implementacion_IA",
        "C:/Carlens/Codigo-03",
    };
    for (const char* candidate : kAbsoluteCandidates) {
        const QString root = matchHistoryRoot(QDir(QString::fromLatin1(candidate)));
        if (!root.isEmpty())
            return root;
    }

    return QCoreApplication::applicationDirPath();
}

QString RemoteHistoryBackend::resolvePythonExe(const QString& projectRoot) const
{
    const QString venvPy = QDir(projectRoot).filePath(QStringLiteral(".venv/Scripts/python.exe"));
    if (QFileInfo::exists(venvPy))
        return venvPy;
    return QStringLiteral("python");
}

QString RemoteHistoryBackend::resolveHelperPath(const QString& projectRoot) const
{
    const QString helper = QDir(projectRoot).filePath(QString::fromLatin1(kHelperRelPath));
    return QFileInfo::exists(helper) ? helper : QString();
}

QString RemoteHistoryBackend::resolveConfigPath(const QString& projectRoot) const
{
    return QDir(projectRoot).filePath(QString::fromLatin1(kConfigRelPath));
}
