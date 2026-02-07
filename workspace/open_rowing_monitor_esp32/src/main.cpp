#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_heap.h>

// Module Headers
#include "RowingSettings.h"
#include "RowingEngine.h"
// #include "GpioTimerService.h"
// #include "FakeISR.h"
#include "InputTimerService.h"
#include "BleManager.h"
#include "FTMS.h"
#include "RowerBridge.h"
#include "version.h"

#ifdef CONFIG_SYSM_ENABLE_MONITORING
#include "SystemMonitor.h"
#endif

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

K_EVENT_DEFINE(mainLoopEvent);
#define BLE_CONNECTED_EVENT     BIT(0)
#define BLE_DISCONNECTED_EVENT  BIT(1)

void printStartupBanner() {
    LOG_INF("╔════════════════════════════════════════════╗");
    LOG_INF("║   Open Rowing Monitor - ESP32              ║");
    LOG_INF("║   Version: %s                              ║", ORM_VERSION_STRING);
    LOG_INF("║   Build: %s (%s)                           ║", ORM_BUILD_TYPE, ORM_BUILD_DATE);
    LOG_INF("╚════════════════════════════════════════════╝");
    LOG_INF("");
}

void printSystemInfo() {
    LOG_INF("System Information:");
    #ifdef CONFIG_SYS_HEAP_RUNTIME_STATS
    // Get the system heap (Zephyr's main heap for k_malloc, etc.)
    extern struct sys_heap _system_heap;

    struct sys_memory_stats stats;
    int ret = sys_heap_runtime_stats_get(&_system_heap, &stats);

    if (ret == 0) {
        LOG_INF("  Heap Free: %u bytes", stats.free_bytes);
    } else {
        LOG_WRN("Failed to get heap stats (error %d)", ret);
    }
    #else
    LOG_DBG("Heap runtime stats not enabled (CONFIG_SYS_HEAP_RUNTIME_STATS=n)");
    #endif
    LOG_INF("  Main Stack: %u bytes", CONFIG_MAIN_STACK_SIZE);
    LOG_INF("  Physics Stack: %u bytes", CONFIG_INPUT_PHYSICS_THREAD_STACK_SIZE);
    // LOG_INF("  Physics Stack: %u bytes", CONFIG_FAKEISR_PHYSICS_THREAD_STACK_SIZE);
    LOG_INF("");
}

int main(void)
{
    // Print banner
    printStartupBanner();

    // k_event_init(&mainLoopEvent);

    LOG_INF("Initializing hardware...");

    // 1. Settings & Engine
    RowingSettings settings;
    RowingEngine engine(settings);

    // 2. Hardware Timer Service
    InputTimerService inputService(engine);
    if (inputService.init() != 0) {
        LOG_ERR("Failed to initialize GPIO. Check Devicetree alias 'impulse-sensor'");
        return 0;
    }
    // FakeISR fakeisr(engine);

    // 3. BLE Services & Manager
    FTMS ftmsService;
    ftmsService.init();

    BleManager bleManager;
    bleManager.init(&mainLoopEvent);

    // 4. The Bridge
    RowerBridge bridge(engine, ftmsService, bleManager);
    bridge.init();

#ifdef CONFIG_SYSM_ENABLE_MONITORING
    // 5. System Monitoring (Debug builds only)
    SystemMonitor monitor;
    monitor.init();
    monitor.registerThread(k_current_get(), "main_thread");
    monitor.registerThread(inputService.getPhysicsThread(), "physics_thread");
    LOG_INF("System monitoring enabled (debug build)");
#endif

    // Print system info
    printSystemInfo();

    LOG_INF("✓ All systems operational. Ready to row.");
    LOG_INF("Advertising as: %s", CONFIG_BT_DEVICE_NAME);
    LOG_INF("");

    // ================================================================
    // Main Loop
    // ================================================================
    while(1) {
        // Outer Loop
        // Block this thread indefinitely until a connection happens
        // This prevents the main thread from polling continously just to check if there is a connection
        // Event group is handled by BLE Manager

        LOG_INF("Waiting for BLE connection.");
        uint32_t connectedEvent = k_event_wait(&mainLoopEvent, BLE_CONNECTED_EVENT, true, K_FOREVER);
        if(connectedEvent & BLE_CONNECTED_EVENT) {
            LOG_INF("=== SESSION STARTED ===");
            inputService.resume();
            // fakeisr.start();
            engine.startSession();
        }
        while(1) {
            // Inner Loop
            // Active session, do all the work needed.

            bridge.update();
            // k_msleep(250);

#ifdef CONFIG_SYSM_ENABLE_MONITORING
            // System Monitoring (every 30 seconds, debug builds only)
            monitor.update(30000);
#endif
            uint32_t disconnectedEvent = k_event_wait(&mainLoopEvent, BLE_DISCONNECTED_EVENT, true, K_MSEC(250));
            if(disconnectedEvent & BLE_DISCONNECTED_EVENT) {
            LOG_INF("=== SESSION ENDED ===");
            inputService.pause();
            // fakeisr.stop();
            engine.endSession();
            break;
            }
        }
    }

    return 0;
}
