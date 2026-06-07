#pragma once

#include <QDateTime>
#include <QString>

struct CapturedVehicleSnapshot;

struct SnapshotPersistenceContext {
    QString projectRoot;
};

struct ProcessingRunInfo {
    QString sourcePath;
    QString sourceType;
    QString yoloModel;
    QString trackingModel;
    int frameSkip = 0;
    double confidenceThreshold = 0.0;
    double iouThreshold = -1.0;
    QString notes;
    qint64 createdByUserId = -1;
    QDateTime startedAtUtc;
};

struct ProcessingRunCompletionInfo {
    QString status;
    QDateTime endedAtUtc;
};

struct SnapshotPersistenceResult {
    bool success = false;
    QString errorMessage;
    QString warningMessage;
    QString infoMessage;
};

class SnapshotPersistenceBackend {
public:
    virtual ~SnapshotPersistenceBackend() = default;

    virtual SnapshotPersistenceResult beginRun(
        const ProcessingRunInfo &runInfo,
        const SnapshotPersistenceContext &context
    ) {
        Q_UNUSED(runInfo);
        Q_UNUSED(context);

        SnapshotPersistenceResult result;
        result.success = true;
        return result;
    }

    virtual SnapshotPersistenceResult finishRun(
        const ProcessingRunCompletionInfo &completionInfo,
        const SnapshotPersistenceContext &context
    ) {
        Q_UNUSED(completionInfo);
        Q_UNUSED(context);

        SnapshotPersistenceResult result;
        result.success = true;
        return result;
    }

    virtual SnapshotPersistenceResult persist(
        CapturedVehicleSnapshot *snapshot,
        const SnapshotPersistenceContext &context
    ) = 0;
};