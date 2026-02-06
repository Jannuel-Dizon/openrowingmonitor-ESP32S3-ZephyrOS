#include "InputTimerService.h"
#include <zephyr/logging/log.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>

LOG_MODULE_REGISTER(InputTimerService, LOG_LEVEL_INF);

#ifndef CONFIG_INPUT_PHYSICS_THREAD_STACK_SIZE
#define CONFIG_INPUT_PHYSICS_THREAD_STACK_SIZE 4096  // Safe default
#endif

K_THREAD_STACK_DEFINE(physicsThreadStack, CONFIG_INPUT_PHYSICS_THREAD_STACK_SIZE);

#define PHYSICS_PRIORITY 5

// 1. GLOBAL STATIC POINTER (Singleton-ish access for ISR)
static InputTimerService* instance = nullptr;

InputTimerService::InputTimerService(RowingEngine& eng)
    : m_engine(eng),
      lastCycleTime(0),
      isFirstPulse(true),
      isPaused(true) {

          instance = this;

          k_msgq_init(&impulseQueue, impulseQueueBuffer, sizeof(uint32_t), IMPULSE_QUEUE_SIZE);

          k_thread_create(&physicsThreadData,
                          physicsThreadStack,
                          K_THREAD_STACK_SIZEOF(physicsThreadStack),
                          physicsThreadEntryPoint,
                          this, NULL, NULL,
                          PHYSICS_PRIORITY,
                          0,
                          K_NO_WAIT);
      }

int InputTimerService::init() {
    // Register callback for input events
    int ret = input_callback_set(DEVICE_DT_GET(DT_ALIAS(impulse_sensor)),
                                  inputCallback,
                                  this);

    if (ret < 0) {
        LOG_ERR("Failed to register input callback: %d", ret);
        return ret;
    }

    LOG_INF("InputTimerService initialized");
    return 0;
}

void InputTimerService::pause() {
    isPaused = true;
    LOG_INF("Physics Engine PAUSED");
}

void InputTimerService::resume() {
    isPaused = false;
    isFirstPulse = true; // Reset state
    LOG_INF("Physics Engine RESUMED");
}

void InputTimerService::physicsThreadEntryPoint(void* p1, void* p2, void* p3) {
    InputTimerService* self = static_cast<InputTimerService*>(p1);
    self->physicsLoop();
}

void GpioTimerService::physicsLoop() {
    uint32_t deltaCycles;
    LOG_INF("Physics loop thread started");

    while (true) {
        if (k_msgq_get(&impulseQueue, &deltaCycles, K_FOREVER) == 0) {

            double dt = (double)deltaCycles / (double)sys_clock_hw_cycles_per_sec();
            engine.handleRotationImpulse(dt);
        }
    }
}

void InputTimerService::inputCallback(struct input_event *evt, void *user_data) {
    InputTimerService* self = static_cast<InputTimerService*>(user_data);

    // We only care about key press events (rising edge)
    // For a reed switch, this is when the magnet passes
    if (evt->code != INPUT_KEY_0 || evt->type != INPUT_EV_KEY) {
        return; // Not our sensor
    }

    // Only process on "press" (magnet detected)
    // evt->value: 0 = released, 1 = pressed
    if (evt->value != 1) {
        return;
    }

    // If paused, ignore
    if (self->isPaused) {
        return;
    }

    // Get current time
    uint32_t currentCycles = k_cycle_get_32();

    if (self->isFirstPulse) {
        self->lastCycleTime = currentCycles;
        self->isFirstPulse = false;
        return;
    }

    // Calculate dt
    uint32_t deltaCycles = currentCycles - self->lastCycleTime;
    self->lastCycleTime = currentCycles;

    k_msgq_put(&impulseQueue, &deltaCycles, K_NO_WAIT);
}
