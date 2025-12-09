#include "MovingAverager.h"

// Constructor
MovingAverager::MovingAverager(int len, std::vector<double> initValue) {
    this->length = len;
    this->dataPoints = initValue;
}

void MovingAverager::pushValue(double dataPoint) {
    // JS logic: Moves all elements to the right, inserts new one at [0]
    // In C++, inserting at [0] of a vector is slow (O(N)).
    // Since this array is small (usually length < 10), it's fine to just loop.

    for (int i = length - 1; i > 0; i--) {
        dataPoints[i] = dataPoints[i - 1];
    }
    dataPoints[0] = dataPoint;
}

double MovingAverager::getAverage() {
    double sum = 0.0;
    for (double val : dataPoints) {
        sum += val;
    }
    return sum / length;
}

void MovingAverager::replaceLastPushedValue(double dataPoint) {
    dataPoints[0] = dataPoint;
}

void MovingAverager::reset(double initValue) {
    dataPoints.assign(length, initValue);
}
