#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

// Include your module headers
#include "RowingSettings.h"
#include "RowingEngine.h"
#include "GpioTimerService.h"
#include "BleManager.h"
#include "FTMS.h" // NOTE: See "Crucial Fix" below
#include "RowerBridge.h"

// Register the main module for logging
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
    LOG_INF("Starting Open Rowing Monitor ...");

    // --------------------------------------------------------------------------
    // 1. Initialize Settings & Physics Engine
    // --------------------------------------------------------------------------
    // This loads defaults from Kconfig (RowingSettings.h)
    RowingSettings settings;

    // Instantiate the Physics Engine with those settings
    RowingEngine engine(settings);

    // --------------------------------------------------------------------------
    // 2. Initialize Hardware (GPIO / Timers)
    // --------------------------------------------------------------------------
    // Inject the engine into the GPIO service so it knows where to send interrupts
    GpioTimerService gpioService(engine);

    if (gpioService.init() != 0) {
        LOG_ERR("Failed to initialize GPIO Service! System halted.");
        return -1;
    }

    // --------------------------------------------------------------------------
    // 3. Initialize BLE Services (GATT)
    // --------------------------------------------------------------------------
    // Instantiate the FTMS Service.
    // (Note: The GATT service definitions in FTMS.cpp register themselves automatically
    // with Zephyr at boot, but we need this object to update the values later).
    FTMS ftmsService;
    ftmsService.init();

    // --------------------------------------------------------------------------
    // 4. Initialize BLE Manager (Gap / Advertising)
    // --------------------------------------------------------------------------
    // Starts the Bluetooth stack and begins advertising the FTMS UUID
    BleManager bleManager;
    bleManager.init();

    // --------------------------------------------------------------------------
    // 5. Initialize the Bridge
    // --------------------------------------------------------------------------
    // The bridge connects the Physics Engine to the FTMS Service
    RowerBridge bridge(engine, ftmsService);

    LOG_INF("System Initialization Complete. Entering Main Loop.");

    // --------------------------------------------------------------------------
    // 6. Main Loop
    // --------------------------------------------------------------------------
    while (1) {
        // Poll the engine and update BLE if necessary.
        // The bridge handles its own rate limiting (2Hz), so we can call this frequently.
        bridge.update();

        // Sleep to yield the processor to other threads (like the BLE stack
        // or the GPIO physics thread).
        // 10ms = 100Hz loop rate, which is plenty for checking UI/BLE updates.
        k_msleep(10);
    }

    return 0;
}
