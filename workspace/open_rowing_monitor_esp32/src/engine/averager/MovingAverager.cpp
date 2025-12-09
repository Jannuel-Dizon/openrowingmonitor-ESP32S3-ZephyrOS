#include "MovingAverager.h"

// Constructor
MovingAverager::MovingAverager(int len, double initValue) {
    if(len > MAX_AVERAGE_SIZE)
    {
        len = MAX_AVERAGE_SIZE;
    }
    this->capacity = len;
    this->reset(initValue);
}

void MovingAverager::pushValue(double dataPoint) {
    for (int i = capacity - 1; i > 0; i--) {
        this->dataPoints[i] = this->dataPoints[i - 1];
    }
    this->dataPoints[0] = dataPoint;

}

double MovingAverager::getAverage() {
    double sum = 0.0;
    for (double val : this->dataPoints) {
        sum += val;
    }
    return sum / this->capacity;
}

void MovingAverager::replaceLastPushedValue(double dataPoint) {
    this->dataPoints[0] = dataPoint;
}

void MovingAverager::reset(double initValue) {
    for (int i = 0; i < capacity; i++) {
        this->dataPoints[i] = initValue;
    }
}
