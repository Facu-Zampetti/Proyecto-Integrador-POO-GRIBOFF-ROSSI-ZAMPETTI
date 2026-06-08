#pragma once

#include <QImage>

#include "DetectionTypes.h"

struct FramePayload {
    QImage frame;
    DetectionList detections;
    int frameId = 0;
    double timestamp = 0.0;
    int sourceWidth = 0;
    int sourceHeight = 0;
};