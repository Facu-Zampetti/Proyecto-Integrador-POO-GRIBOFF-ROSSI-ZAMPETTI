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

    bool m_initialized = false;
    bool m_enabled = false;
    QString m_pythonExe;
    QString m_helperPath;
    QString m_configPath;
    qint64 m_videoSourceId = -1;
    qint64 m_processingSessionId = -1;
};