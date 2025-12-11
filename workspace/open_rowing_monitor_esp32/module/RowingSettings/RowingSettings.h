#pragma once

#include <zephyr/kernel.h>
#include <cstdint>

/**
 * @brief Configuration struct for the Open Rowing Monitor Physics Engine.
 * * This struct maps Zephyr Kconfig values (defined in module/RowingSettings/Kconfig)
 * to usable C++ types (doubles, bools) for the physics engine.
 * * It handles the conversion from "scaled integers" (used in Kconfig) to
 * actual floating point units (seconds, kg*m^2, etc).
 */
struct RowingSettings {
    // =========================================================
    // 1. Physics Constants
    // =========================================================

    // Number of magnets on the flywheel (default: 1)
    double numOfImpulsesPerRevolution = (double)CONFIG_ORM_IMPULSES_PER_REV;

    // Flywheel Inertia in kg*m^2.
    // Kconfig uses x10000 scaling (e.g., 600 -> 0.06 kg*m^2)
    double flywheelInertia = (double)CONFIG_ORM_FLYWHEEL_INERTIA_X10000 / 10000.0;

    // Magic Constant for distance calculation.
    // Kconfig uses x10000 scaling (e.g. 28000 -> 2.8)
    // Note: We default to 2.8 (Concept2 standard) if not defined.
    #ifdef CONFIG_ORM_MAGIC_CONSTANT_X10000
    double magicConstant = (double)CONFIG_ORM_MAGIC_CONSTANT_X10000 / 10000.0;
    #else
    double magicConstant = 2.8;
    #endif

    // =========================================================
    // 2. Timing & Validation Limits (Seconds)
    // =========================================================

    // Shortest valid time between magnets. Filters switch bounce/noise.
    double minimumTimeBetweenImpulses = (double)CONFIG_ORM_MIN_TIME_BETWEEN_IMPULSE_X10000 / 10000.0;

    // Longest valid time between magnets. Slower than this = Pause/Stop.
    double maximumTimeBetweenImpulses = (double)CONFIG_ORM_MAX_TIME_BETWEEN_IMPULSE_X10000 / 10000.0;

    // Minimum duration required for a valid Drive Phase.
    double minimumDriveTime = (double)CONFIG_ORM_MIN_DRIVE_TIME_X10000 / 10000.0;

    // Minimum duration required for a valid Recovery Phase.
    double minimumRecoveryTime = (double)CONFIG_ORM_MIN_RECOVERY_TIME_X10000 / 10000.0;

    // Max time allowed for a single impulse before assuming the user paused the workout.
    double maximumImpulseTimeBeforePause = (double)CONFIG_ORM_MAX_IMPULSE_TIME_BEFORE_PAUSE_X10000 / 10000.0;

    // =========================================================
    // 3. Flank Detection & Noise Filters
    // =========================================================

    // Size of the moving average buffer (e.g., 4 samples).
    int smoothing = CONFIG_ORM_SMOOTHING;

    // Number of samples to confirm a phase change (Drive <-> Recovery).
    int flankLength = CONFIG_ORM_FLANK_LENGTH;

    // Error tolerance for noise in direction detection.
    int numberOfErrorsAllowed = CONFIG_ORM_NUM_OF_ERRORS_ALLOWED;

    // Max allowed acceleration % per impulse (e.g. 0.25 = 25%).
    double maximumDownwardChange = (double)CONFIG_ORM_MAXIMUM_DOWNWARD_CHANGE_X10000 / 10000.0;

    // Max allowed deceleration % per impulse (e.g. 1.75 = 175%).
    double maximumUpwardChange = (double)CONFIG_ORM_MAXIMUM_UPWARD_CHANGE_X10000 / 10000.0;

    // Natural deceleration of flywheel (if using pure physics detection).
    double naturalDeceleration = (double)CONFIG_ORM_NATURAL_DECELARATION_X10000 / 10000.0;

    // =========================================================
    // 4. Drag Factor Logic
    // =========================================================

    // Base Drag Factor.
    // Note: JS engine divides by 1,000,000.
    // If Kconfig is 1500, this becomes 0.0015.
    double dragFactor = (double)CONFIG_ORM_DRAG_FACTOR / 1000000.0;

    // Check if Auto-Adjust feature is enabled in Kconfig
    bool autoAdjustDragFactor = IS_ENABLED(CONFIG_ORM_AUTO_ADJUST_DRAG_FACTOR);

    // Conditional members: These only exist or have valid values if Auto-Adjust is ON.
    // We use pre-processor directives to provide safe defaults if the config is missing.
    #ifdef CONFIG_ORM_AUTO_ADJUST_DRAG_FACTOR
        int dampingConstantSmoothing = CONFIG_ORM_DAMPING_CONSTANT_SMOOTING;
        // Max change allowed in Drag Factor (e.g. 0.1 = 10%)
        double dampingConstantMaxChange = (double)CONFIG_ORM_DAMPING_CONSTANT_MAX_CHANGE_X10000 / 10000.0;
    #else
        // Dummy defaults to prevent compilation errors if referenced
        int dampingConstantSmoothing = 1;
        double dampingConstantMaxChange = 0.0;
    #endif
};
