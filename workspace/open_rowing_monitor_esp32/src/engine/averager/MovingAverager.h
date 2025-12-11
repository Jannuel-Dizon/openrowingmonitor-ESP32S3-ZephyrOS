#pragma once
#include <algorithm>
#include <zephyr/kernel.h> // Required to see CONFIG_ macros

// ----------------------------------------------------------------------
// Calculate the exact memory needed at Compile Time
// ----------------------------------------------------------------------

// 1. Determine the size needed for Drag Factor (if enabled)
#ifdef CONFIG_ORM_AUTO_ADJUST_DRAG_FACTOR
    #define _DRAG_SIZE CONFIG_ORM_DAMPING_CONSTANT_SMOOTING
#else
    #define _DRAG_SIZE 0
#endif

// 2. Determine the size needed for Flank Detection
#define _FLANK_SIZE CONFIG_ORM_SMOOTHING

// 3. Set the Capacity to the LARGER of the two
#if _DRAG_SIZE > _FLANK_SIZE
    #define MAX_AVERAGER_CAPACITY _DRAG_SIZE
#else
    #define MAX_AVERAGER_CAPACITY _FLANK_SIZE
#endif

// ----------------------------------------------------------------------

class MovingAverager {
private:
    // Now this array is exactly as big as it needs to be, and no bigger.
    double dataPoints[MAX_AVERAGER_CAPACITY];
    int length;
    double currAve;

public:
    MovingAverager(int requestedLength, double initValue);
    void pushValue(double dataPoint);
    void replaceLastPushedValue(double dataPoint);
    double getAverage();
    void reset(double initValue);
};
