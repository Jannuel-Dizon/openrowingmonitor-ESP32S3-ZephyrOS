#pragma once

#include <zephyr/kernel.h>
#include "RowingSettings.h"
#include "MovingFlankDetector.h"
#include "RowingData.h"
#include "MovingAverager.h"

// Interface for Observer
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

    // Data Protection
    mutable k_mutex dataLock; // Mutable allows locking even in 'const' functions
    RowingData currentData;

    RowingEngineObserver* observer = nullptr;

    // Internal State
    double angularDisplacementPerImpulse;
    double drivePhaseStartTime = 0;
    double drivePhaseStartAngularDisplacement = 0;
    double recoveryPhaseStartTime = 0;
    double recoveryPhaseStartAngularDisplacement = 0;
    double previousAngularVelocity = 0;

    // Helpers
    double calculateLinearVelocity(double driveAngle, double recoveryAngle, double cycleTime);
    double calculateCyclePower(double driveAngle, double recoveryAngle, double cycleTime);
    double calculateTorque(double dt, double currentVel);

    void startDrivePhase(double dt);
    void updateDrivePhase(double dt);
    void startRecoveryPhase(double dt);
    void updateRecoveryPhase(double dt);

public:
    explicit RowingEngine(RowingSettings settings);

    void handleRotationImpulse(double dt);
    void setObserver(RowingEngineObserver* obs);
    void reset();

    // Thread-Safe Accessor
    RowingData getData();
};
