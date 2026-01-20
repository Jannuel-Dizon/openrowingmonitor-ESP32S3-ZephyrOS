#pragma once

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/poweroff.h>

// ESP32-specific deep sleep headers
#include <esp_sleep.h>
#include <driver/rtc_io.h>

/**
 * @brief Power states for the rowing monitor
 */
enum class PowerState {
    DEEP_SLEEP,      // Ultra-low power, mode on button only
    AWAKE,           // Advertising, waiting for connection or activity
    IN_SESSION       // Connected and actively monitoring
};

/**
 * @brief Manages power states and deep sleep transitions
 *
 * State Machine:
 * - DEEP_SLEEP → (button press) → AWAKE
 * - AWAKE → (timeout + no activity) → DEEP_SLEEP
 * - AWAKE → (BLE connect) → IN_SESSION
 * - IN_SESSION → (BLE disconnect all) → AWAKE
 */
class PowerManager {
public:
    PowerManager();

    /**
     * @brief Initialize power management (call after BLE/GPIO init)
     */
    void init();

    /**
     * @brief Update state machine (call from main loop)
     */
    void update();

    /**
     * @brief Notify that BLE client connected
     */
    void notifyBleConnected();

    /**
     * @brief Notify that BLE client disconnected
     * @param allDisconnected True if no clients remain
     */
    void notifyBleDisconnected(bool allDisconnected);

    /**
     * @brief Notify that rowing activity detected
     */
    void notifyActivity();

    /**
     * @brief Get current power state
     */
    PowerState getState() const { return currentState; }

    /**
     * @brief Force transition to deep sleep (for testing)
     */
    void forceSleep();

private:
    PowerState currentState;

    // Timers
    uint32_t lastActivityTime;
    uint32_t lastConnectionTime;

    // Timeouts (configurable via Kconfig)
    uint32_t inactivityTimeout;  // ms to sleep after no activity

    // GPIO for mode button
    struct gpio_dt_spec modeButton;
    struct gpio_callback buttonCallback;

    // State transition handlers
    void enterDeepSleep();
    void enterAwake();
    void enterSession();

    // Button interrupt handler
    static void buttonPressedStatic(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
    void buttonPressed();

    // Check if we should auto-sleep
    bool shouldAutoSleep();
};
