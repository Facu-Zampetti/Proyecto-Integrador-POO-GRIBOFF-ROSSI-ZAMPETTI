#include "CompositeSnapshotPersistenceBackend.h"

CompositeSnapshotPersistenceBackend::CompositeSnapshotPersistenceBackend(
    std::unique_ptr<SnapshotPersistenceBackend> primaryBackend,
    std::unique_ptr<SnapshotPersistenceBackend> secondaryBackend
)
    : primaryBackend_(std::move(primaryBackend)),
      secondaryBackend_(std::move(secondaryBackend)) {
}

SnapshotPersistenceResult CompositeSnapshotPersistenceBackend::beginRun(
    const ProcessingRunInfo &runInfo,
    const SnapshotPersistenceContext &context
) {
    SnapshotPersistenceResult primaryResult = primaryBackend_->beginRun(runInfo, context);
    if (!primaryResult.success) {
        return primaryResult;
    }

    const SnapshotPersistenceResult secondaryResult = secondaryBackend_->beginRun(runInfo, context);
    return mergeSecondaryResult(primaryResult, secondaryResult);
}

SnapshotPersistenceResult CompositeSnapshotPersistenceBackend::finishRun(
    const ProcessingRunCompletionInfo &completionInfo,
    const SnapshotPersistenceContext &context
) {
    SnapshotPersistenceResult primaryResult = primaryBackend_->finishRun(completionInfo, context);
    if (!primaryResult.success) {
        return primaryResult;
    }

    const SnapshotPersistenceResult secondaryResult =
        secondaryBackend_->finishRun(completionInfo, context);
    return mergeSecondaryResult(primaryResult, secondaryResult);
}

SnapshotPersistenceResult CompositeSnapshotPersistenceBackend::persist(
    CapturedVehicleSnapshot *snapshot,
    const SnapshotPersistenceContext &context
) {
    SnapshotPersistenceResult primaryResult = primaryBackend_->persist(snapshot, context);
    if (!primaryResult.success) {
        return primaryResult;
    }

    const SnapshotPersistenceResult secondaryResult = secondaryBackend_->persist(snapshot, context);
    return mergeSecondaryResult(primaryResult, secondaryResult);
}

SnapshotPersistenceResult CompositeSnapshotPersistenceBackend::mergeSecondaryResult(
    const SnapshotPersistenceResult &primaryResult,
    const SnapshotPersistenceResult &secondaryResult
) const {
    SnapshotPersistenceResult result = primaryResult;
    if (!secondaryResult.success) {
        result.warningMessage = secondaryResult.errorMessage;
        return result;
    }

    if (!secondaryResult.warningMessage.isEmpty()) {
        result.warningMessage = secondaryResult.warningMessage;
    }

    if (!secondaryResult.infoMessage.isEmpty()) {
        result.infoMessage = secondaryResult.infoMessage;
    }

    return result;
}