#pragma once

enum class RowingState {
    IDLE,
    DRIVE,
    RECOVERY
};

struct RowingData {
    // Current State
    RowingState state = RowingState::IDLE;

    // Time
    double totalTime = 0.0;          // Seconds since start
    double lastStrokeTime = 0.0;     // Duration of the previous cycle
    double driveDuration = 0.0;      // Duration of current/last drive
    double recoveryDuration = 0.0;   // Duration of current/last recovery

    // Physics
    double dragFactor = 0.0;        // Drag Coefficient

    // Instantaneous data
    double distance = 0.0;      // Total Meters
    double instSpeed = 0.0;     // m/s (Average for the stroke)
    double instPower = 0.0;     // Watts (Average for the stroke)

    // Cumulative Data for Averages
    double totalSpmSum = 0.0;
    double totalSpeedSum = 0.0;
    double totalPowerSum = 0.0;
    uint32_t strokeSampleCount = 0;     // Number of strokes recorded in session

    // Calculated Averages for BLE
    double avgSpm = 0.0;
    double avgSpeed = 0.0;
    double avgPower = 0.0;

    // Live Data (High Frequency)
    double instTorque = 0.0;        // For Force Curve
    double angularAcceleration = 0.0;
    double spm = 0.0;               // Strokes Per Minute
    int strokeCount = 0;

    // Training Session State
    bool sessionActive = false;
    double sessionStartTime = 0.0;

};
