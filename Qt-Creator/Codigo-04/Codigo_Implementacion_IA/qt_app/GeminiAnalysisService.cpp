#include "GeminiAnalysisService.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTemporaryFile>

namespace {
QString resolvePythonExe(const QString &projectRoot) {
    const QString venvPy = QDir(projectRoot).filePath(".venv/Scripts/python.exe");
    if (QFileInfo::exists(venvPy)) {
        return venvPy;
    }
    return QStringLiteral("python");
}

QString resolveHelperPath(const QString &projectRoot) {
    return QDir(projectRoot).filePath("backend/gemini_vehicle_analysis.py");
}

QString resolveConfigPath(const QString &projectRoot) {
    return QDir(projectRoot).filePath("config/vehicle_surveillance.remote.json");
}

QString resolveRemoteSyncHelperPath(const QString &projectRoot) {
    return QDir(projectRoot).filePath("backend/remote_snapshot_sync.py");
}

VehicleAnalysisResult uploadAnalysisToRemote(
    const QString &pythonExe,
    const QString &remoteSyncHelperPath,
    const QString &configPath,
    const CapturedVehicleSnapshot &snapshot,
    const QString &analysisOutputPath
) {
    VehicleAnalysisResult result;

    if (snapshot.processingSessionId < 0 || snapshot.databaseTrackId < 0 || snapshot.remoteFilePath.isEmpty()) {
        result.warningMessage = QStringLiteral(
            "[AI] El analisis se guardo localmente, pero no hay contexto remoto suficiente para subirlo al VPS."
        );
        return result;
    }

    QTemporaryFile payloadFile;
    payloadFile.setAutoRemove(true);
    if (!payloadFile.open()) {
        result.warningMessage = QStringLiteral(
            "[AI] El analisis se genero, pero no se pudo crear el payload temporal para subirlo al VPS."
        );
        return result;
    }

    QJsonObject payload;
    payload.insert("processing_session_id", static_cast<qint64>(snapshot.processingSessionId));
    payload.insert("track_id", static_cast<qint64>(snapshot.databaseTrackId));
    payload.insert("snapshot_id", snapshot.snapshotId);
    payload.insert("local_analysis_path", analysisOutputPath);

    payloadFile.write(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    payloadFile.flush();

    QProcess process;
    process.setProgram(pythonExe);
    process.setArguments(
        {
            remoteSyncHelperPath,
            QStringLiteral("upload-analysis"),
            QStringLiteral("--config"),
            configPath,
            QStringLiteral("--payload"),
            payloadFile.fileName(),
        }
    );
    process.start();

    if (!process.waitForStarted(5000)) {
        result.warningMessage = QStringLiteral(
            "[AI] El analisis se genero, pero no se pudo iniciar la subida del JSON al VPS."
        );
        return result;
    }

    if (!process.waitForFinished(30000)) {
        process.kill();
        process.waitForFinished(2000);
        result.warningMessage = QStringLiteral(
            "[AI] El analisis se genero, pero la subida del JSON al VPS excedio el tiempo limite."
        );
        return result;
    }

    const QByteArray stdOut = process.readAllStandardOutput().trimmed();
    const QByteArray stdErr = process.readAllStandardError().trimmed();
    if (process.exitCode() != 0) {
        result.warningMessage = stdErr.isEmpty()
                                    ? QStringLiteral(
                                          "[AI] El analisis se genero, pero el helper remoto fallo al subir el JSON."
                                      )
                                    : QString("[AI] %1").arg(QString::fromUtf8(stdErr));
        return result;
    }

    QJsonParseError parseError;
    const QJsonDocument responseDoc = QJsonDocument::fromJson(stdOut, &parseError);
    if (parseError.error != QJsonParseError::NoError || !responseDoc.isObject()) {
        result.warningMessage = QStringLiteral(
            "[AI] El analisis se genero, pero la respuesta remota no se pudo interpretar."
        );
        return result;
    }

    const QJsonObject response = responseDoc.object();
    const QJsonObject data = response.value("data").toObject();
    result.remoteOutputPath = data.value("remote_analysis_path").toString();
    return result;
}
}

VehicleAnalysisResult GeminiAnalysisService::analyzeSnapshot(
    const CapturedVehicleSnapshot &snapshot,
    const QString &projectRoot
) const {
    VehicleAnalysisResult result;

    if (projectRoot.isEmpty()) {
        result.errorMessage = QStringLiteral("[AI] No se pudo resolver la raiz del proyecto.");
        return result;
    }

    if (snapshot.localImagePath.isEmpty() || !QFileInfo::exists(snapshot.localImagePath)) {
        result.errorMessage = QStringLiteral("[AI] No existe una imagen local valida para analizar.");
        return result;
    }

    const QString pythonExe = resolvePythonExe(projectRoot);
    const QString helperPath = resolveHelperPath(projectRoot);
    const QString remoteSyncHelperPath = resolveRemoteSyncHelperPath(projectRoot);
    const QString configPath = resolveConfigPath(projectRoot);
    const QString outputPath = QDir(projectRoot).filePath(
        QStringLiteral("captures/%1.analysis.json").arg(snapshot.snapshotId)
    );

    if (!QFileInfo::exists(helperPath)) {
        result.errorMessage = QStringLiteral("[AI] No se encontro el helper de Gemini.");
        return result;
    }

    if (!QFileInfo::exists(configPath)) {
        result.warningMessage = QStringLiteral(
            "[AI] No existe config/vehicle_surveillance.remote.json; Gemini queda deshabilitado."
        );
        return result;
    }

    QProcess process;
    process.setProgram(pythonExe);
    process.setWorkingDirectory(projectRoot);
    process.setArguments(
        {
            helperPath,
            QStringLiteral("--config"),
            configPath,
            QStringLiteral("--image"),
            snapshot.localImagePath,
            QStringLiteral("--snapshot-id"),
            snapshot.snapshotId,
            QStringLiteral("--label"),
            snapshot.label,
            QStringLiteral("--output"),
            outputPath,
        }
    );
    process.start();

    if (!process.waitForStarted(5000)) {
        result.errorMessage = QStringLiteral("[AI] No se pudo iniciar el helper Python de Gemini.");
        return result;
    }

    if (!process.waitForFinished(45000)) {
        process.kill();
        process.waitForFinished(2000);
        result.errorMessage = QStringLiteral("[AI] Gemini no respondio a tiempo.");
        return result;
    }

    const QByteArray stdOut = process.readAllStandardOutput().trimmed();
    const QByteArray stdErr = process.readAllStandardError().trimmed();

    QJsonParseError parseError;
    const QJsonDocument responseDoc = QJsonDocument::fromJson(stdOut, &parseError);
    if (parseError.error != QJsonParseError::NoError || !responseDoc.isObject()) {
        result.errorMessage = stdErr.isEmpty()
                                  ? QStringLiteral("[AI] La respuesta de Gemini no se pudo interpretar.")
                                  : QString::fromUtf8(stdErr);
        return result;
    }

    const QJsonObject response = responseDoc.object();
    result.configured = response.value("configured").toBool(false);
    if (!response.value("success").toBool(false)) {
        const QString helperError = response.value("error").toString();
        if (!result.configured) {
            result.warningMessage = QString("[AI] %1").arg(helperError);
        } else {
            result.errorMessage = QString("[AI] %1").arg(helperError);
        }
        return result;
    }

    const QJsonObject analysis = response.value("analysis").toObject();
    result.success = true;
    result.outputPath = response.value("output_path").toString(outputPath);
    result.make = analysis.value("make").toString();
    result.model = analysis.value("model").toString();
    result.color = analysis.value("color").toString();
    result.bodyType = analysis.value("body_type").toString();
    result.vehicleType = analysis.value("vehicle_type").toString();
    result.yearRange = analysis.value("year_range").toString();
    result.summary = analysis.value("summary").toString();
    result.confidence = analysis.value("confidence").toDouble(-1.0);

    const VehicleAnalysisResult remoteUploadResult = uploadAnalysisToRemote(
        pythonExe,
        remoteSyncHelperPath,
        configPath,
        snapshot,
        result.outputPath
    );
    result.remoteOutputPath = remoteUploadResult.remoteOutputPath;
    result.warningMessage = remoteUploadResult.warningMessage;
    result.infoMessage = result.remoteOutputPath.isEmpty()
                             ? QStringLiteral("[AI] Analisis Gemini completado correctamente.")
                             : QStringLiteral(
                                   "[AI] Analisis Gemini completado y subido al VPS correctamente."
                               );
    return result;
}