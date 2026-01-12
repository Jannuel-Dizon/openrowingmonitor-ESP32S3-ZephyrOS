#pragma once

#include <zephyr/kernel.h>
#include "RowingSettings.h"
#include "MovingFlankDetector.h"
#include "RowingData.h"
#include "MovingAverager.h"

class RowingEngine {
private:
    RowingSettings settings;
    MovingFlankDetector flankDetector;
    MovingAverager dragFactorAverager;

    // Data Protection
    mutable k_mutex dataLock; // Mutable allows locking even in 'const' functions
    RowingData currentData;

    // Internal State
    double angularDisplacementPerImpulse;
    double drivePhaseStartTime = 0;
    double drivePhaseStartAngularDisplacement = 0;
    double recoveryPhaseStartTime = 0;
    double recoveryPhaseStartAngularDisplacement = 0;
    double previousAngularVelocity = 0;

    // Automatic dragfactor
    double recoveryDragAccumulator = 0.0;
    int recoveryDragSampleCount = 0;

    // Helpers
    double calculateLinearVelocity(double driveAngle, double recoveryAngle, double cycleTime);
    double calculateCyclePower(double driveAngle, double recoveryAngle, double cycleTime);
    double calculateTorque(double dt, double currentVel, double alpha);

    void startDrivePhase(double dt);
    void updateDrivePhase(double dt);
    void startRecoveryPhase(double dt);
    void updateRecoveryPhase(double dt);
    void resetSessionInternal();
public:
    explicit RowingEngine(RowingSettings settings);
    void startSession();
    void endSession();

    void handleRotationImpulse(double dt);
    void reset();

    // Thread-Safe Accessor
    RowingData getData();
    void printData();
};
