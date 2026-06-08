#include "CaptureManager.h"

#include <QDateTime>

#include "CompositeSnapshotPersistenceBackend.h"
#include "FileSystemSnapshotPersistenceBackend.h"
#include "RemoteSnapshotPersistenceBackend.h"

CaptureManager::CaptureManager()
    : CaptureManager(std::make_unique<CompositeSnapshotPersistenceBackend>(
          std::make_unique<FileSystemSnapshotPersistenceBackend>(),
          std::make_unique<RemoteSnapshotPersistenceBackend>())) {
}

CaptureManager::CaptureManager(std::unique_ptr<SnapshotPersistenceBackend> persistenceBackend)
    : persistenceBackend_(std::move(persistenceBackend)) {
}

void CaptureManager::setProjectRoot(const QString &projectRoot) {
    persistenceContext_.projectRoot = projectRoot;
}

SnapshotPersistenceResult CaptureManager::beginProcessingRun(const ProcessingRunInfo &runInfo) {
    return persistenceBackend_->beginRun(runInfo, persistenceContext_);
}

SnapshotPersistenceResult CaptureManager::finishProcessingRun(
    const ProcessingRunCompletionInfo &completionInfo
) {
    return persistenceBackend_->finishRun(completionInfo, persistenceContext_);
}

CaptureResult CaptureManager::prepareDetection(
    const QImage &frame,
    int frameId,
    double timestamp,
    const DetectionBox &detection,
    const QPoint &imagePoint
) const {
    CaptureResult result;

    if (persistenceContext_.projectRoot.isEmpty() || frame.isNull()) {
        result.errorMessage = QStringLiteral("[ERR] No hay un frame activo para guardar la captura.");
        return result;
    }

    if (detection.confidence < minimumConfidenceForCapture()) {
        result.errorMessage = QStringLiteral(
            "[ERR] La deteccion no tiene suficiente confianza para capturar."
        );
        return result;
    }

    const QRect imageBounds(QPoint(0, 0), frame.size());
    const QRect boundedBox = detection.bbox.intersected(imageBounds);
    if (!boundedBox.isValid() || boundedBox.isEmpty()) {
        result.errorMessage = QStringLiteral("[ERR] La bbox seleccionada esta fuera del frame.");
        return result;
    }

    result.snapshot = buildSnapshot(frame, frameId, timestamp, detection, imagePoint, boundedBox);

    result.success = true;
    return result;
}

CaptureResult CaptureManager::persistSnapshot(CapturedVehicleSnapshot snapshot) {
    CaptureResult result;
    result.snapshot = std::move(snapshot);

    const SnapshotPersistenceResult persistenceResult =
        persistenceBackend_->persist(&result.snapshot, persistenceContext_);
    if (!persistenceResult.success) {
        result.errorMessage = persistenceResult.errorMessage;
        return result;
    }

    result.success = true;
    result.warningMessage = persistenceResult.warningMessage;
    result.infoMessage = persistenceResult.infoMessage;
    return result;
}

CaptureResult CaptureManager::saveDetection(
    const QImage &frame,
    int frameId,
    double timestamp,
    const DetectionBox &detection,
    const QPoint &imagePoint
) {
    CaptureResult preparedResult = prepareDetection(
        frame,
        frameId,
        timestamp,
        detection,
        imagePoint
    );
    if (!preparedResult.success) {
        return preparedResult;
    }

    return persistSnapshot(std::move(preparedResult.snapshot));
}

CapturedVehicleSnapshot CaptureManager::buildSnapshot(
    const QImage &frame,
    int frameId,
    double timestamp,
    const DetectionBox &detection,
    const QPoint &imagePoint,
    const QRect &boundedBox
) const {
    const QDateTime capturedAtUtc = QDateTime::currentDateTimeUtc();
    const QString safeLabel = detection.label.isEmpty() ? QStringLiteral("vehicle")
                                                        : QString(detection.label).replace(' ', '_');

    CapturedVehicleSnapshot snapshot;
    snapshot.snapshotId = QString("capture_f%1_%2_%3")
                              .arg(frameId, 6, 10, QChar('0'))
                              .arg(safeLabel)
                              .arg(capturedAtUtc.toString("yyyyMMdd_hhmmss_zzz"));
    snapshot.frameId = frameId;
    snapshot.timestamp = timestamp;
    snapshot.classId = detection.classId;
    snapshot.label = detection.label;
    snapshot.confidence = detection.confidence;
    snapshot.bbox = boundedBox;
    snapshot.imageSize = frame.size();
    snapshot.clickPoint = imagePoint;
    snapshot.trackId = detection.trackId;
    snapshot.capturedAtUtc = capturedAtUtc;
    snapshot.croppedImage = frame.copy(boundedBox);
    return snapshot;
}