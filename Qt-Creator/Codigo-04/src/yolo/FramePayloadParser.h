#pragma once

#include <QString>

#include "FramePayload.h"

class QJsonObject;

class FramePayloadParser {
public:
    static bool parse(const QJsonObject &payload, FramePayload *framePayload, QString *errorMessage);
};