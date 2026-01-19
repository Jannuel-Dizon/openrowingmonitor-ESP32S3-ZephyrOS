#include "GpioTimerService.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(GpioTimerService, LOG_LEVEL_INF);

#ifndef CONFIG_PHYSICS_THREAD_STACK_SIZE
#define CONFIG_PHYSICS_THREAD_STACK_SIZE 4096  // Safe default
#endif

K_THREAD_STACK_DEFINE(physicsThreadStack, CONFIG_PHYSICS_THREAD_STACK_SIZE);

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

    ret = gpio_pin_interrupt_configure_dt(&sensorSpec, GPIO_INT_DISABLE);
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
    LOG_INF("Physics loop thread started");

    // Monitoring Variables
    uint32_t impulseCount = 0;
    uint32_t lastMonitorTime = k_uptime_get_32();
    size_t minStackFree = SIZE_MAX;

    #ifdef CONFIG_ENABLE_PHYSICS_PROFILING
    uint32_t maxProcessingTime = 0;
    uint32_t totalProcessingTime = 0;
    #endif

    while (true) {
        if (k_msgq_get(&impulseQueue, &dt, K_FOREVER) == 0) {
            impulseCount++;

            #ifdef CONFIG_ENABLE_PHYSICS_PROFILING
            uint32_t startCycles = k_cycle_get_32();
            #endif

            // === THE ACTUAL WORK ===
            engine.handleRotationImpulse(dt);

            #ifdef CONFIG_ENABLE_PHYSICS_PROFILING
            uint32_t elapsed = k_cycle_get_32() - startCycles;
            uint32_t elapsedUs = k_cyc_to_us_floor32(elapsed);
            totalProcessingTime += elapsedUs;
            if (elapsedUs > maxProcessingTime) {
                maxProcessingTime = elapsedUs;
                LOG_DBG("New max processing time: %u us", maxProcessingTime);
            }
            #endif

            // ===============================================
            // INLINE STACK MONITORING (Every 50 impulses)
            // ===============================================
            if (impulseCount % 50 == 0) {
                size_t unused;
                if (k_thread_stack_space_get(&physicsThreadData, &unused) == 0) {
                    if (unused < minStackFree) {
                        minStackFree = unused;
                        LOG_WRN("Physics thread LOW WATER MARK: %u bytes free (impulse #%u)",
                                unused, impulseCount);
                    }

                    // Critical warning if below 512 bytes
                    if (unused < 512) {
                        LOG_ERR("CRITICAL: Physics stack almost full! %u bytes remaining!", unused);
                    }
                }
            }

            // ===============================================
            // PERIODIC DETAILED REPORT (Every 30 seconds)
            // ===============================================
            uint32_t now = k_uptime_get_32();
            if ((now - lastMonitorTime) > 30000) {
                LOG_INF("=== Physics Thread Report ===");
                LOG_INF("  Uptime: %u seconds", now / 1000);
                LOG_INF("  Total Impulses: %u", impulseCount);
                LOG_INF("  Stack Low Water Mark: %u bytes", minStackFree);

                #ifdef CONFIG_ENABLE_PHYSICS_PROFILING
                if (impulseCount > 0) {
                    uint32_t avgTime = totalProcessingTime / impulseCount;
                    LOG_INF("  Avg processing time: %u us", avgTime);
                    LOG_INF("  Max processing time: %u us", maxProcessingTime);
                }
                #endif

                LOG_INF("=============================");
                lastMonitorTime = now;
            }
        }
    }
}

void GpioTimerService::interruptHandlerStatic(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
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

void GpioTimerService::pause() {
    gpio_pin_interrupt_configure_dt(&sensorSpec, GPIO_INT_DISABLE);
    LOG_INF("Physics Engine PAUSED (Interrupts disabled)");
}

void GpioTimerService::resume() {
    isFirstPulse = true; // Reset state so the first stroke isn't huge
    gpio_pin_interrupt_configure_dt(&sensorSpec, GPIO_INT_EDGE_TO_ACTIVE);
    LOG_INF("Physics Engine RESUMED");
}

struct k_thread* GpioTimerService::getPhysicsThread() {
    return &physicsThreadData;
}
