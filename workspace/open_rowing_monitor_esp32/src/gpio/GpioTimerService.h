#pragma once

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "../engine/RowingEngine.h"

// Define a safe queue size.
// 10 items is plenty (rowing is ~30 SPM, even with 3 magnets that's slow for a CPU)
#define IMPULSE_QUEUE_SIZE 10

class GpioTimerService {
public:
    // Pass the engine by reference so we can feed it data
    explicit GpioTimerService(RowingEngine& engine);

    // Setup GPIO and Interrupts
    int init();

private:
    RowingEngine& engine;

    // Zephyr GPIO Structures
    struct gpio_dt_spec sensorSpec;
    struct gpio_callback pinCbData;

    // Timing State
    uint32_t lastCycleTime;
    bool isFirstPulse;

    // Thread Communication
    // We use a Message Queue to send 'dt' (double) from ISR to a Worker Thread
    struct k_msgq impulseQueue;
    char __aligned(8) impulseQueueBuffer[IMPULSE_QUEUE_SIZE * sizeof(double)];

    struct k_work_delayable processWork;

    // --------------------------------------------------------
    // ISR (Interrupt Service Routine)
    // --------------------------------------------------------
    // Static trampoline to get back into C++ class context
    static void interruptHandlerStatic(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

    // The actual logic running in interrupt context (KEEP THIS FAST)
    void handleInterrupt();

    // --------------------------------------------------------
    // Worker (Thread Context)
    // --------------------------------------------------------
    // This pops data from the queue and calls the physics engine
    static void workHandlerStatic(struct k_work *work);
    void processQueue();
};
