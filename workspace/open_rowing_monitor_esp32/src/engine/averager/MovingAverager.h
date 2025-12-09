#pragma once
#include <vector>
#include <numeric>

class MovingAverager {
private:
    // These maps to the "let dataPoints" inside the JS function
    std::vector<double> dataPoints;
    int length;

public:
    // This maps to "function createMovingAverager(length, initValue)"
    MovingAverager(int length, std::vector<double> initValue);

    // This maps to "function pushValue(dataPoint)"
    void pushValue(double dataPoint);

    // This maps to "function getAverage()"
    double getAverage();

    // This maps to "function replaceLastPushedValue(dataPoint)"
    void replaceLastPushedValue(double dataPoint);

    // This maps to "function reset()"
    void reset(double initValue);
};
