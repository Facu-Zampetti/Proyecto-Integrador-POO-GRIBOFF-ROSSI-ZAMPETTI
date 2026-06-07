#pragma once

#include <memory>

#include <QImage>
#include <QPoint>
#include <QString>

#include "CapturedVehicleSnapshot.h"
#include "DetectionTypes.h"
#include "SnapshotPersistenceBackend.h"

struct CaptureResult {
    bool success = false;
    CapturedVehicleSnapshot snapshot;
    QString errorMessage;
    QString warningMessage;
    QString infoMessage;
};

class CaptureManager {
public:
    CaptureManager();
    explicit CaptureManager(std::unique_ptr<SnapshotPersistenceBackend> persistenceBackend);

    static constexpr double minimumConfidenceForCapture() {
        return 0.65;
    }

    void setProjectRoot(const QString &projectRoot);
    SnapshotPersistenceResult beginProcessingRun(const ProcessingRunInfo &runInfo);
    SnapshotPersistenceResult finishProcessingRun(const ProcessingRunCompletionInfo &completionInfo);
    CaptureResult prepareDetection(
        const QImage &frame,
        int frameId,
        double timestamp,
        const DetectionBox &detection,
        const QPoint &imagePoint
    ) const;
    CaptureResult persistSnapshot(CapturedVehicleSnapshot snapshot);
    CaptureResult saveDetection(
        const QImage &frame,
        int frameId,
        double timestamp,
        const DetectionBox &detection,
        const QPoint &imagePoint
    );

private:
    CapturedVehicleSnapshot buildSnapshot(
        const QImage &frame,
        int frameId,
        double timestamp,
        const DetectionBox &detection,
        const QPoint &imagePoint,
        const QRect &boundedBox
    ) const;

    SnapshotPersistenceContext m_persistenceContext;
    std::unique_ptr<SnapshotPersistenceBackend> m_persistenceBackend;
};