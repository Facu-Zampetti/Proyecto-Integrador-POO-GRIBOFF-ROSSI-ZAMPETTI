#pragma once

#include "SnapshotPersistenceBackend.h"

class FileSystemSnapshotPersistenceBackend : public SnapshotPersistenceBackend {
public:
    SnapshotPersistenceResult persist(
        CapturedVehicleSnapshot *snapshot,
        const SnapshotPersistenceContext &context
    ) override;
};