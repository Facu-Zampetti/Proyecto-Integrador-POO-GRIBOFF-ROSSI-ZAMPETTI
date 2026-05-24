#pragma once

#include <memory>

#include "SnapshotPersistenceBackend.h"

class CompositeSnapshotPersistenceBackend : public SnapshotPersistenceBackend {
public:
    CompositeSnapshotPersistenceBackend(
        std::unique_ptr<SnapshotPersistenceBackend> primaryBackend,
        std::unique_ptr<SnapshotPersistenceBackend> secondaryBackend
    );

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
    SnapshotPersistenceResult mergeSecondaryResult(
        const SnapshotPersistenceResult &primaryResult,
        const SnapshotPersistenceResult &secondaryResult
    ) const;

    std::unique_ptr<SnapshotPersistenceBackend> primaryBackend_;
    std::unique_ptr<SnapshotPersistenceBackend> secondaryBackend_;
};