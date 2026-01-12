#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

// Module Headers
#include "RowingSettings.h"
#include "RowingEngine.h"
#include "GpioTimerService.h"
#include "BleManager.h"
#include "FTMS.h"
#include "RowerBridge.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
    LOG_INF("Starting Open Rowing Monitor (ESP32S3)...");

    // 1. Settings & Engine
    RowingSettings settings;
    RowingEngine engine(settings);

    // 2. Hardware Timer Service
    // This constructor starts the physics thread, and .init() sets up the ISR
    GpioTimerService gpioService(engine);
    if (gpioService.init() != 0) {
        LOG_ERR("Failed to initialize GPIO. Check Devicetree alias 'impulse_sensor'");
        return -1;
    }

    // 3. BLE Services & Manager
    FTMS ftmsService;
    ftmsService.init();

    BleManager bleManager;
    bleManager.init(); // Starts advertising FTMS UUID

    // 4. The Bridge
    // Connects the engine data to the BLE service
    RowerBridge bridge(engine, ftmsService, bleManager);
    bridge.init();

    LOG_INF("All systems go. Ready to row.");

    // 6. Main Loop (Application Thread)
    bool wasConnected = false;
    while (1) {
        bool isConnected = bleManager.isConnected();
        // Handle State Transitions for Physics Engine
        if (isConnected && !wasConnected) {
            gpioService.resume();
            engine.startSession();
            wasConnected = true;
        } else if (!isConnected && wasConnected) {
            gpioService.pause();
            engine.endSession();
            wasConnected = false;
        }

        // Only update the bridge/send BLE data if connected
        if (isConnected) {
            bridge.update();
        }

        k_msleep(500); // Poll at 2Hz
        // k_sleep(K_USEC(12500));
    }
    return 0;
};
