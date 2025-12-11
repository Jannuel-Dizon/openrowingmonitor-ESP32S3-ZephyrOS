#include "RowingEngine.h"
#include <cmath>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(RowingEngine, LOG_LEVEL_INF);

RowingEngine::RowingEngine(RowingSettings rs)
    : settings(rs),
      flankDetector(rs),
      dragFactorAverager(rs.dampingConstantSmoothing, rs.dragFactor) { // Init with Kconfig Drag Factor

    angularDisplacementPerImpulse = (2.0 * 3.14159265359) / settings.numOfImpulsesPerRevolution;
    reset();
}

void RowingEngine::setObserver(RowingEngineObserver* obs) {
    this->observer = obs;
}

void RowingEngine::reset() {
    currentData = RowingData();
    currentData.dragFactor = settings.dragFactor; // Start with static default
    dragFactorAverager.reset(settings.dragFactor);

    // Initialize state to Recovery so the first pull triggers a Drive
    currentData.state = RowingState::RECOVERY;

    // Pre-load plausible values to prevent division by zero on first stroke
    double plausibleDisplacement = 8.0 / std::pow(settings.dragFactor / settings.magicConstant, 1.0/3.0);

    // Fake a previous recovery phase
    recoveryPhaseStartTime = -2.0 * settings.minimumRecoveryTime;
    recoveryPhaseStartAngularDisplacement = -1.0 * (2.0/3.0) * plausibleDisplacement / angularDisplacementPerImpulse;

    previousAngularVelocity = 0;
}

void RowingEngine::handleRotationImpulse(double dt) {
    // 1. Pause Detection
    if (dt > settings.maximumImpulseTimeBeforePause) {
        LOG_INF("Pause detected (dt: %fs)", dt);
        // We don't have a "HandlePause" yet, but we effectively reset or idle
        return;
    }

    currentData.totalTime += dt;

    // 2. Feed the Detector
    flankDetector.pushValue(dt);

    // 3. State Machine
    if (currentData.state == RowingState::DRIVE) {
        // We are Driving. Should we switch to Recovery?
        if (flankDetector.isFlywheelUnpowered()) {
            double driveLen = (currentData.totalTime - flankDetector.timeToBeginOfFlank()) - drivePhaseStartTime;

            if (driveLen >= settings.minimumDriveTime) {
                // Valid Drive -> Switch to Recovery
                startRecoveryPhase(dt);
                currentData.state = RowingState::RECOVERY;
            } else {
                // Too short, keep driving (but log it debug)
                updateDrivePhase(dt);
            }
        } else {
            // Still powering
            updateDrivePhase(dt);
        }
    } else {
        // We are Recovering. Should we switch to Drive?
        if (flankDetector.isFlywheelPowered()) {
            double recLen = (currentData.totalTime - flankDetector.timeToBeginOfFlank()) - recoveryPhaseStartTime;

            if (recLen >= settings.minimumRecoveryTime) {
                // Valid Recovery -> Switch to Drive
                startDrivePhase(dt);
                currentData.state = RowingState::DRIVE;
            } else {
                updateRecoveryPhase(dt);
            }
        } else {
            updateRecoveryPhase(dt);
        }
    }
}

void RowingEngine::startDrivePhase(double dt) {
    // 1. Finalize the previous Recovery Phase
    double endTime = currentData.totalTime - flankDetector.timeToBeginOfFlank();
    double recoveryLen = endTime - recoveryPhaseStartTime;
    double driveLen = currentData.driveDuration;

    // 2. Calculate Cycle Metrics (if valid)
    if (recoveryLen >= settings.minimumRecoveryTime && driveLen >= settings.minimumDriveTime) {
        double cycleTime = driveLen + recoveryLen;
        currentData.lastStrokeTime = cycleTime;
        currentData.spm = 60.0 / cycleTime;

        // Auto-Adjust Drag Factor Logic
        // We calculate drag based on how much it slowed down during recovery
        double recoveryStartImpulse = flankDetector.impulseLengthAtBeginFlank();
        // The current impulse (dt) is the end of recovery
        double recoveryEndImpulse = dt; // Approximation using current dt

        if (settings.autoAdjustDragFactor && recoveryStartImpulse > 0 && recoveryEndImpulse > 0) {
             double w_start = angularDisplacementPerImpulse / recoveryStartImpulse;
             double w_end = angularDisplacementPerImpulse / recoveryEndImpulse;

             // Drag Factor Formula: DF = -J * (1/w_start - 1/w_end) / t
             double calculatedDrag = -1.0 * settings.flywheelInertia * ((1.0/w_start) - (1.0/w_end)) / recoveryLen;

             // Filter logic (Bounds check)
             double avgDrag = dragFactorAverager.getAverage();
             double maxUp = avgDrag * (1.0 + settings.maximumUpwardChange);   // e.g. 1.0 + 0.15
             double maxDown = avgDrag * (1.0 - settings.maximumDownwardChange); // e.g. 1.0 - 0.15

             // If plausible, add to averager
             if (calculatedDrag > maxDown && calculatedDrag < maxUp) {
                 dragFactorAverager.pushValue(calculatedDrag);
             } else {
                 // Cap it
                 if (calculatedDrag > maxUp) dragFactorAverager.pushValue(maxUp);
                 else dragFactorAverager.pushValue(maxDown);
             }
             currentData.dragFactor = dragFactorAverager.getAverage();
             LOG_DBG("New Drag Factor: %f", currentData.dragFactor);
        }
    }

    // 3. Start New Drive
    drivePhaseStartTime = endTime;
    drivePhaseStartAngularDisplacement = currentData.totalTime; // Should use impulse count approximation if strict
    // Simplified: Just tracking time for now to keep it simple

    // Reset Counters
    currentData.strokeCount++;
    if (observer) observer->onStrokeStart(currentData);
}

void RowingEngine::updateDrivePhase(double dt) {
    // Calculate Instantaneous Force/Torque
    double currentVel = angularDisplacementPerImpulse / dt;
    currentData.instantaneousTorque = calculateTorque(dt, currentVel);

    if (observer) observer->onMetricsUpdate(currentData);
}

void RowingEngine::startRecoveryPhase(double dt) {
    double endTime = currentData.totalTime - flankDetector.timeToBeginOfFlank();
    currentData.driveDuration = endTime - drivePhaseStartTime;

    // Calculate Power & Distance for the stroke we just finished
    // We need the angular displacement for Drive & Recovery
    // For simplicity in this port: we assume constant speed for the calculated phase
    // (A full port would track exact impulse counts)

    // 1. Calculate Angular Displacement (Radians)
    // We estimate it based on time x avg speed, or better:
    // We should track exact impulse counts.
    // Let's use a robust approximation:
    double driveImpulses = (currentData.driveDuration / dt); // Rough estimate
    double driveAngle = driveImpulses * angularDisplacementPerImpulse;

    // Use last known recovery angle
    double recoveryAngle = (currentData.recoveryDuration / dt) * angularDisplacementPerImpulse;

    double cycleTime = currentData.driveDuration + currentData.recoveryDuration;

    // Calculate Physics
    currentData.speed = calculateLinearVelocity(driveAngle, recoveryAngle, cycleTime);
    currentData.power = calculateCyclePower(driveAngle, recoveryAngle, cycleTime);

    // Update Total Distance
    // Distance = Speed * Time
    double strokeDist = currentData.speed * cycleTime;
    currentData.distance += strokeDist;

    recoveryPhaseStartTime = endTime;

    if (observer) observer->onStrokeEnd(currentData);
}

void RowingEngine::updateRecoveryPhase(double dt) {
    double currentVel = angularDisplacementPerImpulse / dt;
    currentData.instantaneousTorque = calculateTorque(dt, currentVel);
    if (observer) observer->onMetricsUpdate(currentData);
}

// ------------------------------------------------------------------
// Physics Math
// ------------------------------------------------------------------

double RowingEngine::calculateTorque(double dt, double currentVel) {
    // T = I * alpha + C * omega^2
    // alpha = (omega_new - omega_old) / dt
    double alpha = (currentVel - previousAngularVelocity) / dt;
    double torque = settings.flywheelInertia * alpha + currentData.dragFactor * std::pow(currentVel, 2);

    previousAngularVelocity = currentVel;
    return torque;
}

double RowingEngine::calculateLinearVelocity(double driveAngle, double recoveryAngle, double cycleTime) {
    // v = (C/K)^(1/3) * (TotalAngle / Time)
    // K = Magic Constant
    if (cycleTime <= 0) return 0;

    double totalAngle = driveAngle + recoveryAngle;
    double factor = std::pow(currentData.dragFactor / settings.magicConstant, 1.0/3.0);
    return factor * (totalAngle / cycleTime);
}

double RowingEngine::calculateCyclePower(double driveAngle, double recoveryAngle, double cycleTime) {
    // P = C * ((TotalAngle / Time)^3)
    if (cycleTime <= 0) return 0;

    double totalAngle = driveAngle + recoveryAngle;
    double avgAngularVel = totalAngle / cycleTime;
    return currentData.dragFactor * std::pow(avgAngularVel, 3.0);
}
