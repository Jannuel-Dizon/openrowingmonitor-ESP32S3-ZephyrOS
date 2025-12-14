#include "RowingEngine.h"
#include <cmath>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(RowingEngine, LOG_LEVEL_INF);

RowingEngine::RowingEngine(RowingSettings rs)
    : settings(rs),
      flankDetector(rs),
      dragFactorAverager(rs.dampingConstantSmoothing, rs.dragFactor) {

    k_mutex_init(&dataLock);
    angularDisplacementPerImpulse = (2.0 * 3.14159265359) / settings.numOfImpulsesPerRevolution;
    reset();
}

RowingData RowingEngine::getData() {
    k_mutex_lock(&dataLock, K_FOREVER);
    RowingData copy = currentData;
    k_mutex_unlock(&dataLock);
    return copy;
}

void RowingEngine::setObserver(RowingEngineObserver* obs) {
    this->observer = obs;
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
    if (dt > settings.maximumImpulseTimeBeforePause) {
        // Simple log, no logic change needed for pause yet
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
}

void RowingEngine::startDrivePhase(double dt) {
    double endTime = currentData.totalTime - flankDetector.timeToBeginOfFlank();
    double recoveryLen = endTime - recoveryPhaseStartTime;
    double driveLen = currentData.driveDuration;

    k_mutex_lock(&dataLock, K_FOREVER);
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
    if (observer) observer->onStrokeStart(snapshot);
}

void RowingEngine::updateDrivePhase(double dt) {
    double currentVel = angularDisplacementPerImpulse / dt;
    double torque = calculateTorque(dt, currentVel);

    k_mutex_lock(&dataLock, K_FOREVER);
    currentData.instantaneousTorque = torque;
    RowingData snapshot = currentData;
    k_mutex_unlock(&dataLock);

    if (observer) observer->onMetricsUpdate(snapshot);
}

void RowingEngine::startRecoveryPhase(double dt) {
    double endTime = currentData.totalTime - flankDetector.timeToBeginOfFlank();

    k_mutex_lock(&dataLock, K_FOREVER);
    currentData.driveDuration = endTime - drivePhaseStartTime;
    currentData.state = RowingState::RECOVERY;

    double driveImpulses = (currentData.driveDuration / dt);
    double driveAngle = driveImpulses * angularDisplacementPerImpulse;
    double recoveryAngle = (currentData.recoveryDuration / dt) * angularDisplacementPerImpulse;
    double cycleTime = currentData.driveDuration + currentData.recoveryDuration;

    currentData.speed = calculateLinearVelocity(driveAngle, recoveryAngle, cycleTime);
    currentData.power = calculateCyclePower(driveAngle, recoveryAngle, cycleTime);
    currentData.distance += (currentData.speed * cycleTime);

    RowingData snapshot = currentData;
    k_mutex_unlock(&dataLock);

    recoveryPhaseStartTime = endTime;
    if (observer) observer->onStrokeEnd(snapshot);
}

void RowingEngine::updateRecoveryPhase(double dt) {
    double currentVel = angularDisplacementPerImpulse / dt;
    double torque = calculateTorque(dt, currentVel);

    k_mutex_lock(&dataLock, K_FOREVER);
    currentData.instantaneousTorque = torque;
    RowingData snapshot = currentData;
    k_mutex_unlock(&dataLock);

    if (observer) observer->onMetricsUpdate(snapshot);
}

double RowingEngine::calculateTorque(double dt, double currentVel) {
    double alpha = (currentVel - previousAngularVelocity) / dt;
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
