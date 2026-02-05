#include "RowingEngine.h"
#include <cmath>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(RowingEngine, LOG_LEVEL_INF);
static RowingState lastLoggedState = RowingState::RECOVERY;

RowingEngine::RowingEngine(RowingSettings rs)
    : settings(rs),
      flankDetector(rs),
      dragFactorAverager(rs.dampingConstantSmoothing, rs.dragFactor) {

    k_mutex_init(&dataLock);
    angularDisplacementPerImpulse = (2.0 * 3.14159265359) / settings.numOfImpulsesPerRevolution;
    reset();
    printSettings();
    LOG_INF("RowingEngine Initialized");
}

RowingData RowingEngine::getData() {
    k_mutex_lock(&dataLock, K_FOREVER);
    RowingData copy = currentData;
    k_mutex_unlock(&dataLock);
    return copy;
}

void RowingEngine::reset() {
    k_mutex_lock(&dataLock, K_FOREVER);
    currentData = RowingData();
    currentData.dragFactor = settings.dragFactor;
    currentData.state = RowingState::RECOVERY;
    k_mutex_unlock(&dataLock);

    dragFactorAverager.reset(settings.dragFactor);

    double plausibleDisplacement = 8.0 / std::pow(settings.dragFactor / settings.magicConstant, 1.0/3.0);
    recoveryPhaseStartTime = -2.0 * settings.minimumRecoveryTime;
    recoveryPhaseStartAngularDisplacement = -1.0 * (2.0/3.0) * plausibleDisplacement / angularDisplacementPerImpulse;
    previousAngularVelocity = 0;
}

void RowingEngine::handleRotationImpulse(double dt) {
    /* Hmm */
    // LOG_INF("Time between impulses %f", dt);
    // k_mutex_lock(&dataLock, K_FOREVER);
    // currentData.spm = 25.1;
    // currentData.strokeCount = 300;
    // currentData.avgSpm = 25.0;
    // currentData.distance = 2000.2;
    // currentData.instSpeed = 4.2;
    // currentData.avgSpeed = 3.9;
    // currentData.instPower = 101.4;
    // currentData.avgPower = 100.1;
    // currentData.sessionActive = true;
    // currentData.sessionStartTime = 0;
    // k_mutex_unlock(&dataLock);
    /* Hmm */
    /* Main code */
    if (dt < settings.minimumTimeBetweenImpulses) {
        return;
    }
    if (dt > settings.maximumImpulseTimeBeforePause) {
        return;
    }

    k_mutex_lock(&dataLock, K_FOREVER);
    currentData.totalTime += dt;
    RowingState currentState = currentData.state;
    k_mutex_unlock(&dataLock);

    flankDetector.pushValue(dt);

    if (currentState == RowingState::DRIVE) {
        if (flankDetector.isFlywheelUnpowered()) {
            double driveLen = (currentData.totalTime - flankDetector.timeToBeginOfFlank()) - drivePhaseStartTime;
            if (driveLen >= settings.minimumDriveTime) {
                startRecoveryPhase(dt);
            } else {
                updateDrivePhase(dt);
            }
        } else {
            updateDrivePhase(dt);
        }
    } else {
        if (flankDetector.isFlywheelPowered()) {
            double recLen = (currentData.totalTime - flankDetector.timeToBeginOfFlank()) - recoveryPhaseStartTime;
            if (recLen >= settings.minimumRecoveryTime) {
                startDrivePhase(dt);
            } else {
                updateRecoveryPhase(dt);
            }
        } else {
            updateRecoveryPhase(dt);
        }
    }
    /* Main code */
}

void RowingEngine::startDrivePhase(double dt) {
    double endTime = currentData.totalTime - flankDetector.timeToBeginOfFlank();
    double recoveryLen = endTime - recoveryPhaseStartTime;
    double driveLen = currentData.driveDuration;

    if (settings.autoAdjustDragFactor && recoveryDragSampleCount > 0) {
        // 1. Average the samples collected during the last recovery
        double avgDragForLastRecovery = recoveryDragAccumulator / recoveryDragSampleCount;

        // 2. Smooth it using the MovingAverager (prevents jitter)
        dragFactorAverager.pushValue(avgDragForLastRecovery);

        // 3. Update the Settings so the power calc uses the new value
        double smoothedDrag = dragFactorAverager.getAverage();
        settings.dragFactor = smoothedDrag;

        // 4. Update the Data struct so the UI sees the new value
        // (We do this inside the lock below)
    }

    k_mutex_lock(&dataLock, K_FOREVER);
    currentData.dragFactor = settings.dragFactor;
    if (recoveryLen >= settings.minimumRecoveryTime && driveLen >= settings.minimumDriveTime) {
        double cycleTime = driveLen + recoveryLen;
        currentData.lastStrokeTime = cycleTime;
        currentData.spm = 60.0 / cycleTime;
    }
    currentData.state = RowingState::DRIVE;
    currentData.strokeCount++;
    RowingData snapshot = currentData;
    k_mutex_unlock(&dataLock);

    drivePhaseStartTime = endTime;
}

void RowingEngine::updateDrivePhase(double dt) {
    double currentVel = angularDisplacementPerImpulse / dt;
    double alpha = (currentVel - previousAngularVelocity) / dt;
    double torque = calculateTorque(dt, currentVel, alpha);

    k_mutex_lock(&dataLock, K_FOREVER);
    currentData.instTorque = torque;
    currentData.angularAcceleration = alpha;
    RowingData snapshot = currentData;
    k_mutex_unlock(&dataLock);
}

void RowingEngine::startRecoveryPhase(double dt) {
    double endTime = currentData.totalTime - flankDetector.timeToBeginOfFlank();

    k_mutex_lock(&dataLock, K_FOREVER);
    recoveryDragAccumulator = 0.0;
    recoveryDragSampleCount = 0;

    currentData.driveDuration = endTime - drivePhaseStartTime;
    currentData.state = RowingState::RECOVERY;

    // ... (Your physics calculations for speed/power) ...
    double driveImpulses = (currentData.driveDuration / dt);
    double driveAngle = driveImpulses * angularDisplacementPerImpulse;
    double recoveryAngle = (currentData.recoveryDuration / dt) * angularDisplacementPerImpulse;
    double cycleTime = currentData.driveDuration + currentData.recoveryDuration;

    double instSpeed = calculateLinearVelocity(driveAngle, recoveryAngle, cycleTime);
    double instPower = calculateCyclePower(driveAngle, recoveryAngle, cycleTime);

    // 1. AUTO-START LOGIC
    // We check this BEFORE updating averages
    if (!currentData.sessionActive && instSpeed > 0.1) {
        // resetSessionInternal();
        currentData.sessionStartTime = k_uptime_get_32();
        currentData.sessionActive = true;
    }

    // 2. ACCUMULATE SESSION DATA
    // Only happens if the session is actually running
    if (currentData.sessionActive) {
        currentData.instSpeed = instSpeed;
        currentData.instPower = instPower;

        // Accumulate Distance
        currentData.distance += (instSpeed * cycleTime);

        // Update Averages (Stroke-based)
        currentData.strokeSampleCount++;
        currentData.totalSpmSum += currentData.spm;
        currentData.totalSpeedSum += instSpeed;
        currentData.totalPowerSum += instPower;

        currentData.avgSpm = currentData.totalSpmSum / currentData.strokeSampleCount;
        currentData.avgSpeed = currentData.totalSpeedSum / currentData.strokeSampleCount;
        currentData.avgPower = currentData.totalPowerSum / currentData.strokeSampleCount;

        // To handle the time-based inactivity concern:
        // You should also track 'activeRowingTime' here
        // currentData.activeSessionTime += cycleTime;
    }

    k_mutex_unlock(&dataLock);
    recoveryPhaseStartTime = endTime;
}

void RowingEngine::updateRecoveryPhase(double dt) {
    double currentVel = angularDisplacementPerImpulse / dt;
    double alpha = (currentVel - previousAngularVelocity) / dt;

    // Dynamic Drag Factor Logic
    if (settings.autoAdjustDragFactor) {
        // Only calculate if flywheel is actually slowing down (alpha < 0)
        // and moving fast enough to avoid low-speed noise (e.g., > 10 rad/s)
        if (alpha < 0 && currentVel > 10.0) {

            // Physics Formula: k = (I * -alpha) / w^2
            double rawDrag = (settings.flywheelInertia * -alpha) / (currentVel * currentVel);

            // Sanity Check: Ignore wild outliers (e.g. sensor noise causing massive spikes)
            // A drag factor > 0.1 is physically impossible for a rower (usually 0.0001 - 0.005)
            if (rawDrag > 0.0 && rawDrag < 0.1) {
                recoveryDragAccumulator += rawDrag;
                recoveryDragSampleCount++;
            }
        }
    }
    double torque = calculateTorque(dt, currentVel, alpha);


    k_mutex_lock(&dataLock, K_FOREVER);
    currentData.angularAcceleration = alpha;
    currentData.instTorque = torque;
    RowingData snapshot = currentData;
    k_mutex_unlock(&dataLock);
}

double RowingEngine::calculateTorque(double dt, double currentVel, double alpha) {
    double torque = settings.flywheelInertia * alpha + settings.dragFactor * std::pow(currentVel, 2);
    previousAngularVelocity = currentVel;
    return torque;
}

double RowingEngine::calculateLinearVelocity(double driveAngle, double recoveryAngle, double cycleTime) {
    if (cycleTime <= 0) return 0;
    double totalAngle = driveAngle + recoveryAngle;
    double factor = std::pow(settings.dragFactor / settings.magicConstant, 1.0/3.0);
    return factor * (totalAngle / cycleTime);
}

double RowingEngine::calculateCyclePower(double driveAngle, double recoveryAngle, double cycleTime) {
    if (cycleTime <= 0) return 0;
    double totalAngle = driveAngle + recoveryAngle;
    double avgAngularVel = totalAngle / cycleTime;
    return settings.dragFactor * std::pow(avgAngularVel, 3.0);
}

void RowingEngine::resetSessionInternal() {
    currentData = RowingData();
    currentData.dragFactor = settings.dragFactor;
    currentData.state = RowingState::RECOVERY;
    dragFactorAverager.reset(settings.dragFactor);

    // Clear stale drag accumulation from previous session
    recoveryDragAccumulator = 0.0;
    recoveryDragSampleCount = 0;

    // Pre-seed phase timing so first stroke produces valid cycleTime
    double plausibleDisplacement = 8.0 / std::pow(settings.dragFactor / settings.magicConstant, 1.0/3.0);
    recoveryPhaseStartTime = -2.0 * settings.minimumRecoveryTime;
    recoveryPhaseStartAngularDisplacement = -1.0 * (2.0/3.0) * plausibleDisplacement / angularDisplacementPerImpulse;
    previousAngularVelocity = 0;
}

void RowingEngine::startSession() {
    k_mutex_lock(&dataLock, K_FOREVER);
    if (!currentData.sessionActive) {
        LOG_INF("Starting session.");
        resetSessionInternal();
    }
    k_mutex_unlock(&dataLock);
}

void RowingEngine::endSession() {
    k_mutex_lock(&dataLock, K_FOREVER);
    resetSessionInternal();
    LOG_INF("Session ended.");
    k_mutex_unlock(&dataLock);
}

void RowingEngine::printData() {
    k_mutex_lock(&dataLock, K_FOREVER);
    // currentData.spm = 25.1;
    // currentData.strokeCount = 300;
    // currentData.avgSpm = 25.0;
    // currentData.distance = 2000.2;
    // currentData.instSpeed = 4.2;
    // currentData.avgSpeed = 3.9;
    // currentData.instPower = 101.4;
    // currentData.avgPower = 100.1;
    // currentData.sessionActive = true;
    // currentData.sessionStartTime = 0
    printk("\nStroke Rate: %f\n", currentData.spm);
    printk("Stroke Count: %f\n", currentData.strokeCount);
    printk("Average Stroke Rate: %f\n", currentData.avgSpm);
    printk("Distance: %f\n", currentData.distance);
    printk("Pace: %f\n", currentData.instSpeed);
    printk("Average Pace: %f\n", currentData.avgSpeed);
    printk("Power: %f\n", currentData.instPower);
    printk("Average Power: %f\n", currentData.avgPower);
    printk("Drag factor: %f\n", currentData.dragFactor);
    k_mutex_unlock(&dataLock);
}

void RowingEngine::logDragFactor() {
    k_mutex_lock(&dataLock, K_FOREVER);
    // LOG_INF("Drag factor: %f", currentData.dragFactor);
    LOG_INF("Session Active: %d", currentData.sessionActive);
    k_mutex_unlock(&dataLock);
}

void RowingEngine::printSettings() {
    LOG_INF("Fly wheel inertia: %f", settings.flywheelInertia);
    LOG_INF("Magic constant: %f", settings.magicConstant);
    LOG_INF("Drag factor: %f", settings.dragFactor);
};
