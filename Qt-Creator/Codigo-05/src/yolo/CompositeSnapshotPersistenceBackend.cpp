#include "CompositeSnapshotPersistenceBackend.h"

CompositeSnapshotPersistenceBackend::CompositeSnapshotPersistenceBackend(
    std::unique_ptr<SnapshotPersistenceBackend> primaryBackend,
    std::unique_ptr<SnapshotPersistenceBackend> secondaryBackend
)
    : m_primaryBackend(std::move(primaryBackend)),
      m_secondaryBackend(std::move(secondaryBackend)) {
}

SnapshotPersistenceResult CompositeSnapshotPersistenceBackend::beginRun(
    const ProcessingRunInfo &runInfo,
    const SnapshotPersistenceContext &context
) {
    SnapshotPersistenceResult primaryResult = m_primaryBackend->beginRun(runInfo, context);
    if (!primaryResult.success) {
        return primaryResult;
    }

    const SnapshotPersistenceResult secondaryResult = m_secondaryBackend->beginRun(runInfo, context);
    return mergeSecondaryResult(primaryResult, secondaryResult);
}

SnapshotPersistenceResult CompositeSnapshotPersistenceBackend::finishRun(
    const ProcessingRunCompletionInfo &completionInfo,
    const SnapshotPersistenceContext &context
) {
    SnapshotPersistenceResult primaryResult = m_primaryBackend->finishRun(completionInfo, context);
    if (!primaryResult.success) {
        return primaryResult;
    }

    const SnapshotPersistenceResult secondaryResult =
        m_secondaryBackend->finishRun(completionInfo, context);
    return mergeSecondaryResult(primaryResult, secondaryResult);
}

SnapshotPersistenceResult CompositeSnapshotPersistenceBackend::persist(
    CapturedVehicleSnapshot *snapshot,
    const SnapshotPersistenceContext &context
) {
    SnapshotPersistenceResult primaryResult = m_primaryBackend->persist(snapshot, context);
    if (!primaryResult.success) {
        return primaryResult;
    }

    const SnapshotPersistenceResult secondaryResult = m_secondaryBackend->persist(snapshot, context);
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