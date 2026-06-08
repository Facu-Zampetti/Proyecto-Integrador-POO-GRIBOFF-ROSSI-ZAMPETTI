#include "RemoteSnapshotPersistenceBackend.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTemporaryFile>

#include "CapturedVehicleSnapshot.h"

RemoteSnapshotPersistenceBackend::RemoteSnapshotPersistenceBackend() = default;

SnapshotPersistenceResult RemoteSnapshotPersistenceBackend::beginRun(
    const ProcessingRunInfo &runInfo,
    const SnapshotPersistenceContext &context
) {
    SnapshotPersistenceResult readiness = ensureReady(context);
    if (!readiness.success || !enabled_) {
        return readiness;
    }

    QJsonObject payload;
    payload.insert("source_path", runInfo.sourcePath);
    payload.insert("source_type", runInfo.sourceType);
    payload.insert("started_at_utc", runInfo.startedAtUtc.toString(Qt::ISODateWithMs));
    payload.insert("status", QStringLiteral("running"));
    payload.insert("yolo_model", runInfo.yoloModel);
    payload.insert("tracking_model", runInfo.trackingModel);
    payload.insert("frame_skip", runInfo.frameSkip);
    payload.insert("confidence_threshold", runInfo.confidenceThreshold);
    if (runInfo.iouThreshold >= 0.0) {
        payload.insert("iou_threshold", runInfo.iouThreshold);
    } else {
        payload.insert("iou_threshold", QJsonValue::Null);
    }
    payload.insert("notes", runInfo.notes);
    if (runInfo.createdByUserId >= 0) {
        payload.insert("created_by_user_id", static_cast<qint64>(runInfo.createdByUserId));
    } else {
        payload.insert("created_by_user_id", QJsonValue::Null);
    }

    QJsonObject response;
    const SnapshotPersistenceResult helperResult = executeHelper(
        QStringLiteral("start-session"),
        payload,
        &response
    );
    if (!helperResult.success) {
        enabled_ = false;
        return helperResult;
    }

    videoSourceId_ = static_cast<qint64>(response.value("video_source_id").toInteger(-1));
    processingSessionId_ = static_cast<qint64>(response.value("processing_session_id").toInteger(-1));

    SnapshotPersistenceResult result = helperResult;
    result.infoMessage = QString("Sesion remota conectada correctamente. processing_session_id=%1")
                             .arg(processingSessionId_);
    return result;
}

SnapshotPersistenceResult RemoteSnapshotPersistenceBackend::finishRun(
    const ProcessingRunCompletionInfo &completionInfo,
    const SnapshotPersistenceContext &context
) {
    SnapshotPersistenceResult readiness = ensureReady(context);
    if (!readiness.success || !enabled_ || processingSessionId_ < 0) {
        return readiness;
    }

    QJsonObject payload;
    payload.insert("processing_session_id", static_cast<qint64>(processingSessionId_));
    payload.insert("status", completionInfo.status);
    payload.insert("ended_at_utc", completionInfo.endedAtUtc.toString(Qt::ISODateWithMs));

    QJsonObject response;
    const SnapshotPersistenceResult result = executeHelper(
        QStringLiteral("finish-session"),
        payload,
        &response
    );
    SnapshotPersistenceResult finalResult = result;
    processingSessionId_ = -1;
    videoSourceId_ = -1;
    enabled_ = false;
    if (finalResult.success) {
        finalResult.infoMessage = QStringLiteral("Sesion remota cerrada correctamente.");
    }
    return finalResult;
}

SnapshotPersistenceResult RemoteSnapshotPersistenceBackend::persist(
    CapturedVehicleSnapshot *snapshot,
    const SnapshotPersistenceContext &context
) {
    SnapshotPersistenceResult readiness = ensureReady(context);
    if (!readiness.success || !enabled_) {
        return readiness;
    }

    if (snapshot == nullptr) {
        SnapshotPersistenceResult result;
        result.errorMessage = QStringLiteral("[ERR] No hay snapshot para sincronizar remotamente.");
        return result;
    }

    if (processingSessionId_ < 0) {
        SnapshotPersistenceResult result;
        result.errorMessage = QStringLiteral(
            "[ERR] No existe una processing_session remota activa para sincronizar capturas."
        );
        return result;
    }

    QJsonObject snapshotObject;
    snapshotObject.insert("snapshot_id", snapshot->snapshotId);
    snapshotObject.insert("frame_id", snapshot->frameId);
    snapshotObject.insert("timestamp", snapshot->timestamp);
    snapshotObject.insert("captured_at_utc", snapshot->capturedAtUtc.toString(Qt::ISODateWithMs));
    snapshotObject.insert("class_id", snapshot->classId);
    snapshotObject.insert("label", snapshot->label);
    snapshotObject.insert("confidence", snapshot->confidence);
    snapshotObject.insert(
        "bbox",
        QJsonArray{
            snapshot->bbox.left(),
            snapshot->bbox.top(),
            snapshot->bbox.right(),
            snapshot->bbox.bottom()}
    );
    snapshotObject.insert("image_width", snapshot->imageSize.width());
    snapshotObject.insert("image_height", snapshot->imageSize.height());
    snapshotObject.insert("crop_width", snapshot->croppedImage.width());
    snapshotObject.insert("crop_height", snapshot->croppedImage.height());
    snapshotObject.insert("click_point", QJsonArray{snapshot->clickPoint.x(), snapshot->clickPoint.y()});
    if (snapshot->trackId >= 0) {
        snapshotObject.insert("track_id", static_cast<qint64>(snapshot->trackId));
    } else {
        snapshotObject.insert("track_id", QJsonValue::Null);
    }
    snapshotObject.insert("local_image_path", snapshot->localImagePath);
    snapshotObject.insert("snapshot_type", snapshot->snapshotType);
    snapshotObject.insert("is_best", snapshot->isBest);

    QJsonObject payload;
    payload.insert("processing_session_id", static_cast<qint64>(processingSessionId_));
    payload.insert("video_source_id", static_cast<qint64>(videoSourceId_));
    payload.insert("snapshot", snapshotObject);

    QJsonObject response;
    const SnapshotPersistenceResult result = executeHelper(
        QStringLiteral("persist-snapshot"),
        payload,
        &response
    );
    if (!result.success) {
        return result;
    }

    snapshot->processingSessionId = processingSessionId_;
    snapshot->databaseTrackId = static_cast<qint64>(response.value("track_id").toInteger(-1));
    snapshot->databaseDetectionId = static_cast<qint64>(response.value("detection_id").toInteger(-1));
    snapshot->databaseSnapshotId = static_cast<qint64>(response.value("snapshot_id").toInteger(-1));
    snapshot->remoteFilePath = response.value("remote_file_path").toString();

    SnapshotPersistenceResult finalResult = result;
    finalResult.infoMessage = QString(
                                  "Captura sincronizada en el servidor. snapshot_id=%1"
                              )
                                  .arg(snapshot->databaseSnapshotId);
    return finalResult;
}

SnapshotPersistenceResult RemoteSnapshotPersistenceBackend::ensureReady(
    const SnapshotPersistenceContext &context
) {
    SnapshotPersistenceResult result;
    result.success = true;

    if (initialized_) {
        return result;
    }

    pythonExe_ = resolvePythonExe(context.projectRoot);
    helperPath_ = resolveHelperPath(context.projectRoot);
    configPath_ = resolveConfigPath(context.projectRoot);

    if (pythonExe_.isEmpty() || helperPath_.isEmpty()) {
        result.success = false;
        result.errorMessage = QStringLiteral(
            "[ERR] No se pudo resolver el entorno Python o el helper remoto de snapshots."
        );
        initialized_ = true;
        enabled_ = false;
        return result;
    }

    if (!QFileInfo::exists(configPath_)) {
        result.warningMessage = QStringLiteral(
            "Sincronizacion remota deshabilitada: falta config/vehicle_surveillance.remote.json."
        );
        initialized_ = true;
        enabled_ = false;
        return result;
    }

    initialized_ = true;
    enabled_ = true;
    return result;
}

SnapshotPersistenceResult RemoteSnapshotPersistenceBackend::executeHelper(
    const QString &command,
    const QJsonObject &payload,
    QJsonObject *responseObject
) const {
    SnapshotPersistenceResult result;

    QTemporaryFile payloadFile;
    payloadFile.setAutoRemove(true);
    if (!payloadFile.open()) {
        result.errorMessage = QStringLiteral("[ERR] No se pudo crear un archivo temporal para el helper remoto.");
        return result;
    }

    payloadFile.write(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    payloadFile.flush();

    QProcess process;
    process.setProgram(pythonExe_);
    process.setArguments({helperPath_, command, QStringLiteral("--config"), configPath_, QStringLiteral("--payload"), payloadFile.fileName()});
    process.start();

    if (!process.waitForStarted(5000)) {
        result.errorMessage = QStringLiteral("[ERR] No se pudo iniciar el helper Python de persistencia remota.");
        return result;
    }

    if (!process.waitForFinished(30000)) {
        process.kill();
        process.waitForFinished(2000);
        result.errorMessage = QStringLiteral("[ERR] El helper remoto de snapshots no respondio a tiempo.");
        return result;
    }

    const QByteArray stdOut = process.readAllStandardOutput().trimmed();
    const QByteArray stdErr = process.readAllStandardError().trimmed();
    if (process.exitCode() != 0) {
        result.errorMessage = stdErr.isEmpty()
                                  ? QStringLiteral("[ERR] El helper remoto fallo sin detalles adicionales.")
                                  : QString::fromUtf8(stdErr);
        return result;
    }

    QJsonParseError parseError;
    const QJsonDocument responseDoc = QJsonDocument::fromJson(stdOut, &parseError);
    if (parseError.error != QJsonParseError::NoError || !responseDoc.isObject()) {
        result.errorMessage = QStringLiteral(
            "[ERR] La respuesta del helper remoto no es un JSON valido."
        );
        return result;
    }

    const QJsonObject response = responseDoc.object();
    if (!response.value(QStringLiteral("success")).toBool()) {
        result.errorMessage = response.value(QStringLiteral("error")).toString(
            QStringLiteral("[ERR] El helper remoto devolvio un fallo sin detalle.")
        );
        return result;
    }

    if (responseObject != nullptr) {
        *responseObject = response.value(QStringLiteral("data")).toObject();
    }

    result.success = true;
    if (!stdErr.isEmpty()) {
        result.warningMessage = QString::fromUtf8(stdErr);
    }
    return result;
}

QString RemoteSnapshotPersistenceBackend::resolvePythonExe(const QString &projectRoot) const {
    const QString venvPy = QDir(projectRoot).filePath(QStringLiteral(".venv/Scripts/python.exe"));
    if (QFileInfo::exists(venvPy)) {
        return venvPy;
    }
    return QStringLiteral("python");
}

QString RemoteSnapshotPersistenceBackend::resolveHelperPath(const QString &projectRoot) const {
    const QString helper = QDir(projectRoot).filePath(QStringLiteral("backend/remote_snapshot_sync.py"));
    return QFileInfo::exists(helper) ? helper : QString();
}

QString RemoteSnapshotPersistenceBackend::resolveConfigPath(const QString &projectRoot) const {
    return QDir(projectRoot).filePath(QStringLiteral("config/vehicle_surveillance.remote.json"));
}