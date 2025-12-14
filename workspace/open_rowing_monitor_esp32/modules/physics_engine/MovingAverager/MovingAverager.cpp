#include "MovingAverager.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(MovingAverager, LOG_LEVEL_DBG);

MovingAverager::MovingAverager(int requestedLength, double initValue) {
    // Safety Check:
    // If logic somewhere requests a size larger than we allocated,
    // we clamp it to prevent memory corruption (buffer overflow).
    if (requestedLength > MAX_AVERAGER_CAPACITY) {
        LOG_WRN("Requested size %d > Max Capacity %d. Clamping.", requestedLength, MAX_AVERAGER_CAPACITY);
        length = MAX_AVERAGER_CAPACITY;
    } else {
        length = requestedLength;
    }
    reset(initValue);
}

void MovingAverager::pushValue(double dataPoint) {
    // Standard shift logic
    currAve = currAve+((dataPoint-dataPoints[length-1])/length);
    for (int i = length - 1; i > 0; i--) {
        dataPoints[i] = dataPoints[i - 1];
    }
    dataPoints[0] = dataPoint;
}

void MovingAverager::replaceLastPushedValue(double dataPoint) {
    currAve = currAve+((dataPoint-dataPoints[0])/length);
    dataPoints[0] = dataPoint;
}

double MovingAverager::getAverage() {
    return currAve;
}

void MovingAverager::reset(double initValue) {
    for (int i = 0; i < length; i++) {
        dataPoints[i] = initValue;
    }
    currAve = initValue;
}
