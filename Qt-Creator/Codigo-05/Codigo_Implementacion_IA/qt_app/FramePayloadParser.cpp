#include "FramePayloadParser.h"

#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QPoint>

bool FramePayloadParser::parse(
    const QJsonObject &payload,
    FramePayload *framePayload,
    QString *errorMessage
) {
    if (framePayload == nullptr) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("No hay destino para el payload del frame.");
        }
        return false;
    }

    const QByteArray encodedFrame = payload.value("frame_jpeg_base64").toString().toUtf8();
    const QByteArray jpegBytes = QByteArray::fromBase64(encodedFrame);

    QImage frame;
    if (!frame.loadFromData(jpegBytes, "JPG")) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("No se pudo decodificar el frame JPEG recibido.");
        }
        return false;
    }

    DetectionList detections;
    const QJsonArray jsonDetections = payload.value("detections").toArray();
    detections.reserve(jsonDetections.size());

    for (const QJsonValue &value : jsonDetections) {
        const QJsonObject detectionObject = value.toObject();
        const QJsonArray bboxArray = detectionObject.value("bbox").toArray();
        if (bboxArray.size() != 4) {
            continue;
        }

        const int x1 = bboxArray.at(0).toInt();
        const int y1 = bboxArray.at(1).toInt();
        const int x2 = bboxArray.at(2).toInt();
        const int y2 = bboxArray.at(3).toInt();

        DetectionBox detection;
        detection.classId = detectionObject.value("class_id").toInt(-1);
        detection.label = detectionObject.value("label").toString();
        detection.confidence = detectionObject.value("confidence").toDouble();
        detection.bbox = QRect(QPoint(x1, y1), QPoint(x2, y2)).normalized();
        if (detectionObject.contains("track_id") && !detectionObject.value("track_id").isNull()) {
            detection.trackId = static_cast<qint64>(detectionObject.value("track_id").toInteger(-1));
        }
        detections.push_back(detection);
    }

    framePayload->frame = frame;
    framePayload->detections = detections;
    framePayload->frameId = payload.value("frame_id").toInt();
    framePayload->timestamp = payload.value("timestamp").toDouble();
    framePayload->sourceWidth = payload.value("width").toInt(frame.width());
    framePayload->sourceHeight = payload.value("height").toInt(frame.height());
    return true;
}