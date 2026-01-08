#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string>
#include "RowingSettings.h"
#include "RowingEngine.h"
#include "RowingData.h"
#include "GpioTimerService.h"
#include "StorageManager.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

// ------------------------------------------------------------------
// 1. Define an Observer to print data to the console
// ------------------------------------------------------------------
class TerminalLogger : public RowingEngineObserver {
public:
    void onStrokeStart(const RowingData& data) override {
        LOG_INF("--- STROKE START (Count: %d) ---", data.strokeCount);
    }

    void onStrokeEnd(const RowingData& data) override {
        LOG_INF("--- STROKE END ---");
        LOG_INF("  Power:    %.1f Watts", data.power);
        LOG_INF("  Distance: %.1f Meters", data.distance);
        LOG_INF("  SPM:      %.1f", data.spm);
        LOG_INF("  Speed:    %.2f m/s", data.speed);
    }

    void onMetricsUpdate(const RowingData& data) override {
        // This is high frequency (every interrupt), so maybe don't log everything
        // Uncomment to debug live torque
        // LOG_DBG("Torque: %.2f", data.instantaneousTorque);
    }
};

// ------------------------------------------------------------------
// 2. Main Entry Point
// ------------------------------------------------------------------
int main(void) {
    LOG_INF("============================================");
    LOG_INF("   Open Rowing Monitor (ESP32-S3 Zephyr)    ");
    LOG_INF("============================================");

    // 1. Load Settings (Kconfig defaults)
    RowingSettings settings;
    LOG_INF("Settings Loaded:");
    LOG_INF("  Inertia: %f", settings.flywheelInertia);
    LOG_INF("  Drag Factor: %f", settings.dragFactor);
    LOG_INF("  Flank Length: %d", settings.flankLength);

    // 2. Initialize Logic
    RowingEngine engine(settings);
    TerminalLogger logger;
    engine.setObserver(&logger);

    // 3. Initialize Hardware (Starts the Physics Thread on Core 1)
    GpioTimerService sensorService(engine);
    if (sensorService.init() < 0) {
        LOG_ERR("CRITICAL: Sensor setup failed! System halted.");
        return -1;
    }
    StorageManager storage;
        if (storage.init() == 0) {
        	// storage.appendRecord("Booting Open Rowing Monitor...");
            	LOG_INF("Flash initialized.");
        } else {
        	LOG_ERR("Failed");
         	return 0;
        }
    // 4. Main Loop (Core 0)
    // The physics runs in the background. We can use this thread for
    // simple status monitoring or eventually the Bluetooth implementation.
    // storage.listMountedVol();
    // LOG_INF("System Ready! Press the button (GPIO 17) to simulate strokes.");
    storage.dumpFile("test.csv");
    for (int i = 0; i < 50; i++) {
     	// Fetch data safely using the Mutex-protected getter
        RowingData liveData = engine.getData();

        // Print a "Heartbeat" every 5 seconds if idle
        if (liveData.state == RowingState::IDLE) {
            LOG_INF("Status: IDLE | Dist: %.1f m", liveData.distance);
        } else if (liveData.state == RowingState::RECOVERY) {
             LOG_INF("Status: RECOVERY | Torque: %.1f", liveData.instantaneousTorque);
        } else {
             LOG_INF("Status: DRIVE    | Torque: %.1f", liveData.instantaneousTorque);
        }


        // Simple test write every second
        if (liveData.state == RowingState::DRIVE) {
             // Basic CSV format: Time, Power, Distance
             char buffer[64];
             sprintf(buffer, "%.2f, %.1f, %.1f", liveData.totalTime, liveData.power, liveData.distance);
             storage.appendRecord(std::string(buffer));
        }
        k_msleep(1000);
    }
    storage.dumpFile("test.csv");
    return 0;
}
