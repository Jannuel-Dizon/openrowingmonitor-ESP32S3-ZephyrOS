#include "MovingFlankDetector.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(MovingFlankDetector, LOG_LEVEL_DBG);

MovingFlankDetector::MovingFlankDetector(RowingSettings rowerSettings)
    : settings(rowerSettings),
      movingAverage(rowerSettings.smoothing, rowerSettings.maximumTimeBetweenImpulses) {

    angularDisplacementPerImpulse = (2.0 * 3.14159265359) / settings.numOfImpulsesPerRevolution;

    // Initialize arrays with loops instead of .assign()
    double defaultVelocity = angularDisplacementPerImpulse / settings.maximumTimeBetweenImpulses;

    for (int i = 0; i < FLANK_ARRAY_SIZE; i++) {
        dirtyDataPoints[i] = settings.maximumTimeBetweenImpulses;
        cleanDataPoints[i] = settings.maximumTimeBetweenImpulses;
        angularVelocity[i] = defaultVelocity;
        angularAcceleration[i] = 0.1;
    }

    numberOfSequentialCorrections = 0;
    maxNumberOfSequentialCorrections = (settings.smoothing >= 2 ? settings.smoothing : 2);
}

void MovingFlankDetector::pushValue(double dataPoint) {
    // 1. Shift Data
    // Use the Kconfig limit to prevent overflow
    int len = settings.flankLength;
    if (len >= FLANK_ARRAY_SIZE) len = FLANK_ARRAY_SIZE - 1;

    for (int i = len; i > 0; i--) {
        dirtyDataPoints[i] = dirtyDataPoints[i - 1];
        cleanDataPoints[i] = cleanDataPoints[i - 1];
        angularVelocity[i] = angularVelocity[i - 1];
        angularAcceleration[i] = angularAcceleration[i - 1];
    }
    dirtyDataPoints[0] = dataPoint;

    // 2. Noise Filter: Bounds Check
    if (dataPoint < settings.minimumTimeBetweenImpulses || dataPoint > settings.maximumTimeBetweenImpulses) {
        LOG_DBG("Noise Filter: Out of bounds %f", dataPoint);
        dataPoint = cleanDataPoints[1];
    }

    // 3. Noise Filter: Change Limiter
    movingAverage.pushValue(dataPoint);
    double currentAverage = movingAverage.getAverage();
    double previousClean = cleanDataPoints[1];

    bool isPlausible = false;
    if (currentAverage > (settings.maximumDownwardChange * previousClean) &&
        currentAverage < (settings.maximumUpwardChange * previousClean)) {
        isPlausible = true;
    }

    if (isPlausible) {
        numberOfSequentialCorrections = 0;
    } else {
        if (numberOfSequentialCorrections <= maxNumberOfSequentialCorrections) {
            movingAverage.replaceLastPushedValue(previousClean);
            numberOfSequentialCorrections++;
        }
    }

    // 4. Update Derived Metrics
    cleanDataPoints[0] = movingAverage.getAverage();

    if (cleanDataPoints[0] > 0) {
        angularVelocity[0] = angularDisplacementPerImpulse / cleanDataPoints[0];
        angularAcceleration[0] = (angularVelocity[0] - angularVelocity[1]) / cleanDataPoints[0];
    } else {
        angularVelocity[0] = 0;
        angularAcceleration[0] = 0;
    }
}

bool MovingFlankDetector::isFlywheelPowered() {
    int numberOfErrors = 0;
    int len = settings.flankLength;
    if (len >= FLANK_ARRAY_SIZE) len = FLANK_ARRAY_SIZE - 1;

    for (int i = len; i > 1; i--) {
        if (cleanDataPoints[i] < cleanDataPoints[i - 1]) {
            numberOfErrors++;
        }
    }
    if (cleanDataPoints[1] <= cleanDataPoints[0]) {
        numberOfErrors++;
    }
    return (numberOfErrors <= settings.numberOfErrorsAllowed);
}

bool MovingFlankDetector::isFlywheelUnpowered() {
    int numberOfErrors = 0;
    int len = settings.flankLength;
    if (len >= FLANK_ARRAY_SIZE) len = FLANK_ARRAY_SIZE - 1;

    for (int i = len; i > 0; i--) {
        if (cleanDataPoints[i] >= cleanDataPoints[i - 1]) {
            numberOfErrors++;
        }
    }
    return (numberOfErrors <= settings.numberOfErrorsAllowed);
}

double MovingFlankDetector::timeToBeginOfFlank() {
    double total = 0.0;
    int len = settings.flankLength;
    if (len >= FLANK_ARRAY_SIZE) len = FLANK_ARRAY_SIZE - 1;

    for(int i = 0; i <= len; i++) {
        total += dirtyDataPoints[i];
    }
    return total;
}

double MovingFlankDetector::noImpulsesToBeginFlank() {
    return (double)settings.flankLength;
}

double MovingFlankDetector::impulseLengthAtBeginFlank() {
    return cleanDataPoints[settings.flankLength];
}

double MovingFlankDetector::accelerationAtBeginOfFlank() {
    return angularAcceleration[settings.flankLength - 1];
}
