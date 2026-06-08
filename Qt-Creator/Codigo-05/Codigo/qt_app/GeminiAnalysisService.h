#pragma once

#include <QString>

#include "CapturedVehicleSnapshot.h"

struct VehicleAnalysisResult {
    bool success = false;
    bool configured = false;
    QString errorMessage;
    QString warningMessage;
    QString infoMessage;
    QString outputPath;
    QString remoteOutputPath;
    QString make;
    QString model;
    QString color;
    QString bodyType;
    QString vehicleType;
    QString yearRange;
    QString summary;
    double confidence = -1.0;
};

class GeminiAnalysisService {
public:
    VehicleAnalysisResult analyzeSnapshot(
        const CapturedVehicleSnapshot &snapshot,
        const QString &projectRoot
    ) const;
};