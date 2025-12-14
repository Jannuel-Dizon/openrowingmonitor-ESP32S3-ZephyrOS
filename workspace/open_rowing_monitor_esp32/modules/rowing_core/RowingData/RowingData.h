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
    double distance = 0.0;           // Total Meters
    double speed = 0.0;              // m/s (Average for the stroke)
    double power = 0.0;              // Watts (Average for the stroke)
    double dragFactor = 0.0;         // Drag Coefficient

    // Live Data (High Frequency)
    double instantaneousTorque = 0.0;// For Force Curve
    double spm = 0.0;                // Strokes Per Minute
    int strokeCount = 0;
};
