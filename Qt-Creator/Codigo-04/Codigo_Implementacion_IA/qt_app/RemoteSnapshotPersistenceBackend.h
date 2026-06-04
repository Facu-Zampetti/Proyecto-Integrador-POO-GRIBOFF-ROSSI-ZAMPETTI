#pragma once

#include "SnapshotPersistenceBackend.h"

class RemoteSnapshotPersistenceBackend : public SnapshotPersistenceBackend {
public:
    RemoteSnapshotPersistenceBackend();

    SnapshotPersistenceResult beginRun(
        const ProcessingRunInfo &runInfo,
        const SnapshotPersistenceContext &context
    ) override;

    SnapshotPersistenceResult finishRun(
        const ProcessingRunCompletionInfo &completionInfo,
        const SnapshotPersistenceContext &context
    ) override;

    SnapshotPersistenceResult persist(
        CapturedVehicleSnapshot *snapshot,
        const SnapshotPersistenceContext &context
    ) override;

private:
    SnapshotPersistenceResult ensureReady(const SnapshotPersistenceContext &context);
    SnapshotPersistenceResult executeHelper(
        const QString &command,
        const QJsonObject &payload,
        QJsonObject *responseObject
    ) const;
    QString resolvePythonExe(const QString &projectRoot) const;
    QString resolveHelperPath(const QString &projectRoot) const;
    QString resolveConfigPath(const QString &projectRoot) const;

    bool initialized_ = false;
    bool enabled_ = false;
    QString pythonExe_;
    QString helperPath_;
    QString configPath_;
    qint64 videoSourceId_ = -1;
    qint64 processingSessionId_ = -1;
};