#pragma once

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "RowingEngine.h"

#define IMPULSE_QUEUE_SIZE 10

class GpioTimerService {
public:
    explicit GpioTimerService(RowingEngine& engine);
    int init();
    void handleInterrupt();
    void pause();
    void resume();
    struct k_thread* getPhysicsThread();

private:
    RowingEngine& engine;

    // GPIO structs
    struct gpio_dt_spec sensorSpec;
    struct gpio_callback pinCbData;

    // Timing State
    uint32_t lastCycleTime;
    bool isFirstPulse;

    // IPC: Message Queue
    struct k_msgq impulseQueue;
    char __aligned(8) impulseQueueBuffer[IMPULSE_QUEUE_SIZE * sizeof(double)];

    // THREAD DATA
    // We keep the struct here, but the STACK will be defined in the .cpp file
    struct k_thread physicsThreadData;

    // The Loop Function
    void physicsLoop();

    // Static entry points
    static void physicsThreadEntryPoint(void* p1, void* p2, void* p3);
    static void interruptHandlerStatic(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

};
