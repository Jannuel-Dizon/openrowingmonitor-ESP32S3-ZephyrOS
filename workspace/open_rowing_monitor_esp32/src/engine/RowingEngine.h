#pragma once

#include "RowingSettings.h"
#include "MovingFlankDetector.h"
#include "RowingData.h"
#include "averager/MovingAverager.h"

// Interface for anyone who wants to listen to the engine (e.g. Bluetooth, Display)
class RowingEngineObserver {
public:
    virtual void onStrokeStart(const RowingData& data) {}
    virtual void onStrokeEnd(const RowingData& data) {}
    virtual void onMetricsUpdate(const RowingData& data) {}
};

class RowingEngine {
private:
    RowingSettings settings;
    MovingFlankDetector flankDetector;
    MovingAverager dragFactorAverager;

    RowingData currentData;
    RowingEngineObserver* observer = nullptr;

    // Internal State Variables (Ported from JS)
    double angularDisplacementPerImpulse;
    double drivePhaseStartTime = 0;
    double drivePhaseStartAngularDisplacement = 0;
    double recoveryPhaseStartTime = 0;
    double recoveryPhaseStartAngularDisplacement = 0;

    // Helpers
    double calculateLinearVelocity(double driveAngle, double recoveryAngle, double cycleTime);
    double calculateCyclePower(double driveAngle, double recoveryAngle, double cycleTime);
    double calculateTorque(double dt, double currentVel);

    // Phase Management
    void startDrivePhase(double dt);
    void updateDrivePhase(double dt);
    void startRecoveryPhase(double dt);
    void updateRecoveryPhase(double dt);

    // Internal tracking for torque
    double previousAngularVelocity = 0;

public:
    explicit RowingEngine(RowingSettings settings);

    // Main Input
    void handleRotationImpulse(double dt);

    // Setup
    void setObserver(RowingEngineObserver* obs);
    void reset();

    // Accessor
    RowingData getData() const { return currentData; }
};
