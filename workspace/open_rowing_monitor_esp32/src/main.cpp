#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

// Module Headers
#include "RowingSettings.h"
#include "RowingEngine.h"
#include "BleManager.h"
#include "FTMS.h"
#include "RowerBridge.h"
#include "FakeISR.h"


LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

K_EVENT_DEFINE(mainLoopEvent);
#define BLE_CONNECTED_EVENT     BIT(0)
#define BLE_DISCONNECTED_EVENT  BIT(1)

/**
 * TEST 3: FULL SYSTEM WITH REPLAYED DATA
 *
 * This test runs EVERYTHING (physics + BLE), but uses replayed
 * data instead of the real GPIO ISR. If this hangs but Tests 1 & 2
 * didn't, the problem is in the INTERACTION between subsystems.
 */

int main(void)
{
    // LOG_INF("=== FULL SYSTEM WITH REPLAY ===");
    // LOG_INF("Goal: Find interaction bugs");

    // 1. Settings & Engine
    RowingSettings settings;
    RowingEngine engine(settings);

    FakeISR fakeisr(engine);

    // 2. BLE Services & Manager
    FTMS ftmsService;
    ftmsService.init();

    BleManager bleManager;
    bleManager.init(&mainLoopEvent);

    // 3. The Bridge
    RowerBridge bridge(engine, ftmsService, bleManager);
    bridge.init();

    LOG_INF("Loaded %zu test impulses", fakeisr.getDtCount());
    LOG_INF("Waiting for BLE connection...");

    // Main Loop
    while(1) {
        uint32_t connectedEvent = k_event_wait(&mainLoopEvent, BLE_CONNECTED_EVENT, true, K_FOREVER);

        if(connectedEvent & BLE_CONNECTED_EVENT) {
            LOG_INF("=== CONNECTED - Starting full test ===");

            engine.startSession();
            fakeisr.start();

            uint32_t impulse_count = 0;

            while(1) {
                // Update BLE at 4Hz
                bridge.update();

                // Check for disconnect
                uint32_t disconnectedEvent = k_event_wait(&mainLoopEvent, BLE_DISCONNECTED_EVENT, true, K_MSEC(250));
                if(disconnectedEvent & BLE_DISCONNECTED_EVENT) {
                    LOG_INF("=== DISCONNECTED after %u impulses ===", impulse_count);
                    engine.endSession();
                    fakeisr.stop();
                    break;
                }
            }
        }
    }

    // Test for Physics engine without Bluetooth
    // engine.startSession();
    // fakeisr.start();

    // while(1) {
    //     engine.printData();
    //     k_msleep(250);
    // }

    return 0;
}

/**
 * EXPECTED RESULTS:
 *
 * If this HANGS (but 1 & 2 didn't):
 *   - Problem is in how physics and BLE interact
 *   - Possible race condition
 *   - Check mutex usage in RowerBridge::update()
 *   - Check if BLE notifications block physics thread
 *
 * If this WORKS:
 *   - Problem is ONLY in real GPIO ISR
 *   - Likely an interrupt priority issue
 *   - Or message queue overflow
 */
