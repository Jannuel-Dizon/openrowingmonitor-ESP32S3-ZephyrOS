#include "InputTimerService.h"
#include <zephyr/logging/log.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>

LOG_MODULE_REGISTER(InputTimerService, LOG_LEVEL_INF);

#ifndef CONFIG_INPUT_PHYSICS_THREAD_STACK_SIZE
#define CONFIG_INPUT_PHYSICS_THREAD_STACK_SIZE 4096  // Safe default
#endif

static K_THREAD_STACK_DEFINE(physicsThreadStack, CONFIG_INPUT_PHYSICS_THREAD_STACK_SIZE);

#define PHYSICS_PRIORITY 5

// 1. GLOBAL STATIC POINTER (Singleton-ish access for ISR)
static InputTimerService* instance = nullptr;

// 2. Define the static bridge function
static void input_bridge_callback(struct input_event *evt, void *user_data) {
    if (instance) {
        instance->handleInputEvent(evt);
    }
}

INPUT_CALLBACK_DEFINE(NULL,
                      input_bridge_callback,
                      nullptr);
// INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(impulse_sensor)),
//                       input_bridge_callback,
//                       nullptr);

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

void InputTimerService::physicsLoop() {
    uint32_t deltaCycles;
    LOG_INF("Physics loop thread started");

    while (true) {
        if (k_msgq_get(&impulseQueue, &deltaCycles, K_FOREVER) == 0) {

            double dt = (double)deltaCycles / (double)sys_clock_hw_cycles_per_sec();
            m_engine.handleRotationImpulse(dt);
        }
    }
}

void InputTimerService::handleInputEvent(struct input_event *evt) {
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
    if (isPaused) {
        return;
    }

    // Get current time
    uint32_t currentCycles = k_cycle_get_32();

    if (isFirstPulse) {
        lastCycleTime = currentCycles;
        isFirstPulse = false;
        return;
    }

    // Calculate dt
    uint32_t deltaCycles = currentCycles - lastCycleTime;
    lastCycleTime = currentCycles;

    k_msgq_put(&impulseQueue, &deltaCycles, K_NO_WAIT);
}
