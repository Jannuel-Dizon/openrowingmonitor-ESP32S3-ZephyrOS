#include "GpioTimerService.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(GpioTimerService, LOG_LEVEL_INF);

// Helper to get the device tree struct
#define SENSOR_NODE DT_ALIAS(impulse_sensor)

GpioTimerService::GpioTimerService(RowingEngine& eng)
    : engine(eng),
      lastCycleTime(0),
      isFirstPulse(true) {

    // Initialize the GPIO spec from Device Tree
    sensorSpec = GPIO_DT_SPEC_GET(SENSOR_NODE, gpios);

    // Initialize Message Queue
    k_msgq_init(&impulseQueue, impulseQueueBuffer, sizeof(double), IMPULSE_QUEUE_SIZE);

    // Initialize Work Item
    k_work_init_delayable(&processWork, workHandlerStatic);
}

int GpioTimerService::init() {
    if (!gpio_is_ready_dt(&sensorSpec)) {
        LOG_ERR("GPIO device not ready");
        return -1;
    }

    // 1. Configure Pin (Input)
    int ret = gpio_pin_configure_dt(&sensorSpec, GPIO_INPUT);
    if (ret < 0) {
        LOG_ERR("Failed to configure GPIO: %d", ret);
        return ret;
    }

    // 2. Configure Interrupt (Rising Edge = Magnet passing)
    // Note: If you used GPIO_ACTIVE_LOW in overlay, 'Active' means the switch closed.
    // We trigger when it becomes active.
    ret = gpio_pin_interrupt_configure_dt(&sensorSpec, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure Interrupt: %d", ret);
        return ret;
    }

    // 3. Init Callback
    gpio_init_callback(&pinCbData, interruptHandlerStatic, BIT(sensorSpec.pin));
    gpio_add_callback(sensorSpec.port, &pinCbData);

    LOG_INF("GpioTimerService initialized on pin %d", sensorSpec.pin);
    return 0;
}

// ----------------------------------------------------------------
// The ISR: Runs in INTERRUPT CONTEXT. No printing, no float math!
// ----------------------------------------------------------------
void GpioTimerService::interruptHandlerStatic(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    // Get the pointer to the class instance
    GpioTimerService *self = CONTAINER_OF(cb, GpioTimerService, pinCbData);
    self->handleInterrupt();
}

void GpioTimerService::handleInterrupt() {
    // 1. Capture precise hardware cycle count
    uint32_t currentCycles = k_cycle_get_32();

    // 2. Handle first pulse (calibration)
    if (isFirstPulse) {
        lastCycleTime = currentCycles;
        isFirstPulse = false;
        return; // We need two points to measure a duration
    }

    // 3. Calculate Delta Cycles (Handles overflow automatically for unsigned math)
    uint32_t deltaCycles = currentCycles - lastCycleTime;
    lastCycleTime = currentCycles;

    // 4. Convert to Seconds (Double)
    // The ESP32-S3 runs at 240MHz, so this conversion is safe.
    double dt = (double)deltaCycles / (double)sys_clock_hw_cycles_per_sec();

    // 5. Send to Queue (Don't process physics here!)
    // K_NO_WAIT means if the queue is full, we drop the data (prevents ISR lockup)
    if (k_msgq_put(&impulseQueue, &dt, K_NO_WAIT) == 0) {
        // Trigger the worker thread to process the data immediately
        k_work_schedule(&processWork, K_NO_WAIT);
    }
}

// ----------------------------------------------------------------
// The Worker: Runs in THREAD CONTEXT. Physics math is safe here.
// ----------------------------------------------------------------
void GpioTimerService::workHandlerStatic(struct k_work *work) {
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    GpioTimerService *self = CONTAINER_OF(dwork, GpioTimerService, processWork);
    self->processQueue();
}

void GpioTimerService::processQueue() {
    double dt;
    // Process all pending impulses
    while (k_msgq_get(&impulseQueue, &dt, K_NO_WAIT) == 0) {
        // Feed the Physics Engine
        engine.handleRotationImpulse(dt);
    }
}
