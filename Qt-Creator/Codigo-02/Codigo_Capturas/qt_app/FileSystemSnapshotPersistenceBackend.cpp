#include "FileSystemSnapshotPersistenceBackend.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "CapturedVehicleSnapshot.h"

SnapshotPersistenceResult FileSystemSnapshotPersistenceBackend::persist(
    CapturedVehicleSnapshot *snapshot,
    const SnapshotPersistenceContext &context
) {
    SnapshotPersistenceResult result;

    if (snapshot == nullptr) {
        result.errorMessage = QStringLiteral("[ERR] No hay snapshot para persistir.");
        return result;
    }

    if (context.projectRoot.isEmpty()) {
        result.errorMessage = QStringLiteral("[ERR] No se pudo resolver la ruta del proyecto.");
        return result;
    }

    QDir capturesDir(QDir(context.projectRoot).filePath("captures"));
    if (!capturesDir.exists() && !QDir().mkpath(capturesDir.path())) {
        result.errorMessage = QStringLiteral("[ERR] No se pudo crear la carpeta captures/.");
        return result;
    }

    const QString imagePath = capturesDir.filePath(snapshot->snapshotId + ".jpg");
    const QString metadataPath = capturesDir.filePath(snapshot->snapshotId + ".json");

    if (!snapshot->croppedImage.save(imagePath, "JPG", 95)) {
        result.errorMessage = QStringLiteral("[ERR] No se pudo guardar el crop seleccionado.");
        return result;
    }

    QJsonObject metadata;
    metadata.insert("snapshot_id", snapshot->snapshotId);
    metadata.insert("frame_id", snapshot->frameId);
    metadata.insert("timestamp", snapshot->timestamp);
    metadata.insert("captured_at_utc", snapshot->capturedAtUtc.toString(Qt::ISODateWithMs));
    metadata.insert("class_id", snapshot->classId);
    metadata.insert("label", snapshot->label);
    metadata.insert("confidence", snapshot->confidence);
    metadata.insert(
        "bbox",
        QJsonArray{
            snapshot->bbox.left(),
            snapshot->bbox.top(),
            snapshot->bbox.right(),
            snapshot->bbox.bottom()}
    );
    metadata.insert("image_width", snapshot->imageSize.width());
    metadata.insert("image_height", snapshot->imageSize.height());
    metadata.insert("saved_image_path", imagePath);
    metadata.insert("saved_metadata_path", metadataPath);
    metadata.insert("click_point", QJsonArray{snapshot->clickPoint.x(), snapshot->clickPoint.y()});
    metadata.insert("persistence_backend", QStringLiteral("filesystem"));
    if (snapshot->trackId >= 0) {
        metadata.insert("track_id", static_cast<qint64>(snapshot->trackId));
    }

    QFile metadataFile(metadataPath);
    if (!metadataFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        result.errorMessage = QStringLiteral("[ERR] No se pudo guardar el metadata JSON.");
        return result;
    }

    metadataFile.write(QJsonDocument(metadata).toJson(QJsonDocument::Indented));
    metadataFile.close();

    snapshot->localImagePath = imagePath;
    snapshot->localMetadataPath = metadataPath;

    result.success = true;
    return result;
}