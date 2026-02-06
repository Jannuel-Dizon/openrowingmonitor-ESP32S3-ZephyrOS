#pragma once

#include <zephyr/kernel.h>
#include <zephyr/input/input.h>
#include "RowingEngine.h"

#define IMPULSE_QUEUE_SIZE (CONFIG_INPUT_IMPULSE_QUEUE_SIZE * CONFIG_ORM_IMPULSES_PER_REV)

/**
 * @brief Input-based Timer Service for Rowing Monitor
 *
 * Uses Zephyr's Input subsystem instead of raw GPIO interrupts.
 * Benefits:
 * - Hardware debouncing via devicetree
 * - Event processing in dedicated thread (not ISR context)
 * - Built-in queue management
 * - Much simpler code!
 */
class InputTimerService {
public:
    explicit InputTimerService(RowingEngine& engine);
    int init();
    void pause();
    void resume();

private:
    RowingEngine& m_engine;

    // Timing State
    uint32_t lastCycleTime;
    bool isFirstPulse;
    bool isPaused;

    // Impulse queue
    struct k_msgq impulseQueue;
    char __aligned(8) impulseQueueBuffer[IMPULSE_QUEUE_SIZE * sizeof(uint32_t)];

    struct k_thread physicsThreadData;

    void physicsLoop();

    // Input callback
    static void physicsThreadEntryPoint(void *p1, void *p2, void *p3);
    static void inputCallback(struct input_event *evt, void *user_data);
};
