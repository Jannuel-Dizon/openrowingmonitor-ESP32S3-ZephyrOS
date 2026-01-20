#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <esp_sleep.h>  // NEW: For mode cause detection

// Module Headers
#include "RowingSettings.h"
#include "RowingEngine.h"
#include "GpioTimerService.h"
#include "BleManager.h"
#include "FTMS.h"
#include "RowerBridge.h"
#include "SystemMonitor.h"
#include "PowerManager.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
    // ================================================================
    // 0. Check Wake Cause (ESP32 resets on mode from deep sleep)
    // ================================================================
    esp_sleep_wakeup_cause_t wake_cause = esp_sleep_get_wakeup_cause();

    switch (wake_cause) {
        case ESP_SLEEP_WAKEUP_GPIO:
            LOG_INF("=== WOKE FROM DEEP SLEEP (Button Press) ===");
            // You could check which GPIO woke us:
            // uint64_t wakeup_pin_mask = esp_sleep_get_gpio_wakeup_status();
            break;

        case ESP_SLEEP_WAKEUP_TIMER:
            LOG_INF("=== WOKE FROM DEEP SLEEP (Timer) ===");
            break;

        case ESP_SLEEP_WAKEUP_UNDEFINED:
        default:
            LOG_INF("=== COLD BOOT (Not a deep sleep mode) ===");
            break;
    }

    LOG_INF("Starting Open Rowing Monitor (ESP32S3)...");

    // 1. Settings & Engine
    RowingSettings settings;
    RowingEngine engine(settings);

    // 2. Hardware Timer Service
    GpioTimerService gpioService(engine);
    if (gpioService.init() != 0) {
        LOG_ERR("Failed to initialize GPIO. Check Devicetree alias 'impulse_sensor'");
        return -1;
    }

    // 3. BLE Services & Manager
    FTMS ftmsService;
    ftmsService.init();

    BleManager bleManager;
    bleManager.init();

    // 4. The Bridge
    RowerBridge bridge(engine, ftmsService, bleManager);
    bridge.init();

    // 5. System Monitoring
    SystemMonitor monitor;
    monitor.init();
    monitor.registerThread(k_current_get(), "main_thread");
    monitor.registerThread(gpioService.getPhysicsThread(), "physics_thread");

    // ================================================================
    // 6. POWER MANAGEMENT (NEW)
    // ================================================================
    PowerManager powerManager;
    powerManager.init();

    LOG_INF("All systems go. Ready to row.");

    // ================================================================
    // 7. Main Loop
    // ================================================================
    bool wasConnected = false;
    uint32_t strokeCount = 0;
    uint32_t lastStrokeCount = 0;

    while (1) {
        bool isConnected = bleManager.isConnected();

        // ============================================================
        // Handle BLE Connection State Changes
        // ============================================================
        if (isConnected && !wasConnected) {
            // Just connected
            gpioService.resume();
            engine.startSession();
            powerManager.notifyBleConnected();  // NEW!
            wasConnected = true;
            LOG_INF("=== SESSION STARTED ===");
        } else if (!isConnected && wasConnected) {
            // Just disconnected
            gpioService.pause();
            engine.endSession();
            powerManager.notifyBleDisconnected(true);  // NEW!
            wasConnected = false;
            LOG_INF("=== SESSION ENDED ===");
        }

        // ============================================================
        // Update BLE Data & Track Activity
        // ============================================================
        if (isConnected) {
            bridge.update();

            // Track strokes to detect activity
            RowingData data = engine.getData();
            strokeCount = data.strokeCount;

            // Notify power manager of activity
            if (strokeCount > lastStrokeCount) {
                powerManager.notifyActivity();  // NEW!
            }

            // Log progress every 10 strokes
            if (strokeCount > 0 && strokeCount % 10 == 0 && strokeCount != lastStrokeCount) {
                LOG_INF("Progress: %d strokes, %.1f watts, %.0f meters",
                        strokeCount, data.instPower, data.distance);
                lastStrokeCount = strokeCount;
            }
        }

        // ============================================================
        // System Monitoring (every 30 seconds)
        // ============================================================
        monitor.update(30000);

        // ============================================================
        // Power Management Update (NEW)
        // ============================================================
        powerManager.update();

        k_msleep(250);  // Main loop runs at 4Hz
    }

    return 0;
}
