#pragma once

#include <cmath>
#include <cstdint>
#include <zephyr/kernel.h>

#include "averager/MovingAverager.h"
#include "RowingSettings.h"

// Define the array size based on Kconfig.
#define FLANK_ARRAY_SIZE (CONFIG_ORM_FLANK_LENGTH + 1)

class MovingFlankDetector {
private:
    RowingSettings settings;
    MovingAverager movingAverage;

    // Fixed-size arrays (Stack allocated). No std::vector!
    double dirtyDataPoints[FLANK_ARRAY_SIZE];
    double cleanDataPoints[FLANK_ARRAY_SIZE];
    double angularVelocity[FLANK_ARRAY_SIZE];
    double angularAcceleration[FLANK_ARRAY_SIZE];

    double angularDisplacementPerImpulse;
    int numberOfSequentialCorrections;
    int maxNumberOfSequentialCorrections;

public:
    explicit MovingFlankDetector(RowingSettings rowerSettings);

    void pushValue(double dataPoint);

    // State Checks
    bool isFlywheelPowered();
    bool isFlywheelUnpowered();

    // Getters
    double timeToBeginOfFlank();
    double noImpulsesToBeginFlank();
    double impulseLengthAtBeginFlank();
    double accelerationAtBeginOfFlank();
};
