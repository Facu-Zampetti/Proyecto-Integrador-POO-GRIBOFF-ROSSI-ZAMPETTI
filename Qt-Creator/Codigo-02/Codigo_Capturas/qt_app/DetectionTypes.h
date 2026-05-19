#pragma once

#include <QRect>
#include <QString>
#include <QVector>

struct DetectionBox {
    int classId = -1;
    QString label;
    double confidence = 0.0;
    QRect bbox;
    qint64 trackId = -1;
};

using DetectionList = QVector<DetectionBox>;