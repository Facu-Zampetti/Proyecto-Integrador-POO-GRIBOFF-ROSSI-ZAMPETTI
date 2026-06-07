#pragma once

#include <QDateTime>
#include <QImage>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QString>

struct CapturedVehicleSnapshot {
    QString snapshotId;
    int frameId = 0;
    double timestamp = 0.0;
    int classId = -1;
    QString label;
    double confidence = 0.0;
    QRect bbox;
    QSize imageSize;
    QPoint clickPoint;
    qint64 trackId = -1;
    QDateTime capturedAtUtc;
    QImage croppedImage;
    QString snapshotType = QStringLiteral("manual");
    bool isBest = false;
    QString localImagePath;
    QString localMetadataPath;
    QString remoteFilePath;
    qint64 processingSessionId = -1;
    qint64 databaseTrackId = -1;
    qint64 databaseDetectionId = -1;
    qint64 databaseSnapshotId = -1;
};