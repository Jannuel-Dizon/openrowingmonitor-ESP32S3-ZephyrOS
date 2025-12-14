#include "GpioTimerService.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(GpioTimerService, LOG_LEVEL_INF);

K_THREAD_STACK_DEFINE(physicsThreadStack, 2048);

#define PHYSICS_PRIORITY 5

// 1. GLOBAL STATIC POINTER (Singleton-ish access for ISR)
static GpioTimerService* instance = nullptr;

GpioTimerService::GpioTimerService(RowingEngine& eng)
    : engine(eng),
      sensorSpec(GPIO_DT_SPEC_GET(DT_ALIAS(impulse_sensor), gpios)),
      lastCycleTime(0),
      isFirstPulse(true) {

    // Set the global instance to 'this'
    instance = this;

    k_msgq_init(&impulseQueue, impulseQueueBuffer, sizeof(double), IMPULSE_QUEUE_SIZE);

    k_thread_create(&physicsThreadData,
                    physicsThreadStack,
                    K_THREAD_STACK_SIZEOF(physicsThreadStack),
                    physicsThreadEntryPoint,
                    this, NULL, NULL,
                    PHYSICS_PRIORITY,
                    0,
                    K_NO_WAIT);
}

int GpioTimerService::init() {
    if (!gpio_is_ready_dt(&sensorSpec)) {
        LOG_ERR("GPIO device not ready");
        return -1;
    }

    int ret = gpio_pin_configure_dt(&sensorSpec, GPIO_INPUT);
    if (ret < 0) return ret;

    ret = gpio_pin_interrupt_configure_dt(&sensorSpec, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret < 0) return ret;

    gpio_init_callback(&pinCbData, interruptHandlerStatic, BIT(sensorSpec.pin));
    gpio_add_callback(sensorSpec.port, &pinCbData);

    LOG_INF("GpioTimerService initialized on pin %d", sensorSpec.pin);
    return 0;
}

void GpioTimerService::physicsThreadEntryPoint(void* p1, void* p2, void* p3) {
    GpioTimerService* self = static_cast<GpioTimerService*>(p1);
    self->physicsLoop();
}

void GpioTimerService::physicsLoop() {
    double dt;
    while (true) {
        if (k_msgq_get(&impulseQueue, &dt, K_FOREVER) == 0) {
            engine.handleRotationImpulse(dt);
        }
    }
}

// 2. FIXED ISR TRAMPOLINE (No more CONTAINER_OF)
void GpioTimerService::interruptHandlerStatic(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    // Directly use the static instance pointer
    if (instance) {
        instance->handleInterrupt();
    }
}

void GpioTimerService::handleInterrupt() {
    uint32_t currentCycles = k_cycle_get_32();

    if (isFirstPulse) {
        lastCycleTime = currentCycles;
        isFirstPulse = false;
        return;
    }

    uint32_t deltaCycles = currentCycles - lastCycleTime;
    lastCycleTime = currentCycles;

    double dt = (double)deltaCycles / (double)sys_clock_hw_cycles_per_sec();

    k_msgq_put(&impulseQueue, &dt, K_NO_WAIT);
}
