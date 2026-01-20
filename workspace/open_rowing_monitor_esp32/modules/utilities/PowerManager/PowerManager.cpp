#include "PowerManager.h"
#include <zephyr/pm/device.h>

LOG_MODULE_REGISTER(PowerManager, LOG_LEVEL_INF);

// Singleton for ISR access
static PowerManager* instance = nullptr;

PowerManager::PowerManager()
    : currentState(PowerState::AWAKE),
      modeButton(GPIO_DT_SPEC_GET(DT_ALIAS(mode_button), gpios)),
      lastActivityTime(0),
      lastConnectionTime(0),
      inactivityTimeout(CONFIG_POWER_INACTIVITY_TIMEOUT_MS) {

    instance = this;
}

void PowerManager::init() {
    // Configure mode button
    if (!gpio_is_ready_dt(&modeButton)) {
        LOG_ERR("Mode button GPIO not ready!");
        return;
    }

    int ret = gpio_pin_configure_dt(&modeButton, GPIO_INPUT);
    if (ret < 0) {
        LOG_ERR("Failed to configure mode button (error %d)", ret);
        return;
    }

    // Set up interrupt for button press
    ret = gpio_pin_interrupt_configure_dt(&modeButton, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure button interrupt (error %d)", ret);
        return;
    }

    gpio_init_callback(&buttonCallback, buttonPressedStatic, BIT(modeButton.pin));
    gpio_add_callback(modeButton.port, &buttonCallback);

    lastActivityTime = k_uptime_get_32();
    lastConnectionTime = k_uptime_get_32();

    LOG_INF("PowerManager initialized (timeout: %u ms)", inactivityTimeout);
}

void PowerManager::update() {
    switch (currentState) {
        case PowerState::DEEP_SLEEP:
            // Nothing to do - we're asleep!
            // (This state is only entered via enterDeepSleep(), which doesn't return)
            break;

        case PowerState::AWAKE:
            // Check if we should auto-sleep
            if (shouldAutoSleep()) {
                LOG_INF("Inactivity timeout - entering deep sleep");
                enterDeepSleep();
            }
            break;

        case PowerState::IN_SESSION:
            // Stay active - user is rowing or connected
            // Will only exit when notifyBleDisconnected(true) is called
            break;
    }
}

void PowerManager::notifyBleConnected() {
    if (currentState == PowerState::AWAKE) {
        LOG_INF("BLE connected - entering session state");
        enterSession();
    }
    lastConnectionTime = k_uptime_get_32();
}

void PowerManager::notifyBleDisconnected(bool allDisconnected) {
    if (currentState == PowerState::IN_SESSION && allDisconnected) {
        LOG_INF("All BLE clients disconnected - returning to awake state");
        enterAwake();
    }
    lastConnectionTime = k_uptime_get_32();
}

void PowerManager::notifyActivity() {
    lastActivityTime = k_uptime_get_32();

    // If we're somehow in AWAKE but user is rowing, we should know about it
    if (currentState == PowerState::AWAKE) {
        LOG_DBG("Activity detected while awake");
    }
}

bool PowerManager::shouldAutoSleep() {
    uint32_t now = k_uptime_get_32();

    // Only auto-sleep if we're in AWAKE state
    if (currentState != PowerState::AWAKE) {
        return false;
    }

    // Check inactivity timeout
    uint32_t timeSinceActivity = now - lastActivityTime;
    if (timeSinceActivity < inactivityTimeout) {
        return false;
    }

    LOG_INF("Auto-sleep conditions met (inactive for %u ms)", timeSinceActivity);
    return true;
}

void PowerManager::enterDeepSleep() {
    currentState = PowerState::DEEP_SLEEP;

    LOG_INF("=== ENTERING DEEP SLEEP ===");
    LOG_INF("Wake source: GPIO %d (button press)", modeButton.pin);

    // Give logs time to flush
    k_msleep(100);

    // ============================================================
    // ESP32-Specific Deep Sleep Configuration
    // ============================================================

    // Configure GPIO wake source (ESP32 requires special handling)
    // Your button: GPIO 13, PULL_UP, ACTIVE_LOW
    // So we wake when GPIO goes HIGH (button released in active-low config)
    uint64_t wakeup_pin_mask = 1ULL << modeButton.pin;

    // 2. Enable EXT1 wakeup
    // Since your button is Active Low (Pull-up), wake up when it goes LOW
    esp_sleep_enable_ext1_wakeup(wakeup_pin_mask, ESP_EXT1_WAKEUP_ANY_LOW);

    rtc_gpio_pullup_en((gpio_num_t)modeButton.pin);
    rtc_gpio_pulldown_dis((gpio_num_t)modeButton.pin);

    LOG_INF("Configured GPIO %d for wake (active-low)", modeButton.pin);

    // Enter deep sleep using ESP32-specific poweroff
    // This properly shuts down peripherals and enters deep sleep
    sys_poweroff();

    // ============================================================
    // Code below this point will NOT execute
    // ============================================================
    // On wake, ESP32 performs a RESET, so execution starts from main() again
    // You can detect wake reason using: esp_sleep_get_wakeup_cause()
}

void PowerManager::enterAwake() {
    currentState = PowerState::AWAKE;
    lastActivityTime = k_uptime_get_32();
    lastConnectionTime = k_uptime_get_32();
    LOG_INF("Entered AWAKE state");
}

void PowerManager::enterSession() {
    currentState = PowerState::IN_SESSION;
    lastActivityTime = k_uptime_get_32();
    LOG_INF("Entered IN_SESSION state");
}

void PowerManager::forceSleep() {
    LOG_WRN("Force sleep requested");
    enterDeepSleep();
}

void PowerManager::buttonPressedStatic(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    if (instance) {
        instance->buttonPressed();
    }
}

void PowerManager::buttonPressed() {
    LOG_INF("Mode button pressed!");

    switch (currentState) {
        case PowerState::DEEP_SLEEP:
            // Mode from sleep
            LOG_INF("Waking from deep sleep");
            enterAwake();
            break;

        case PowerState::AWAKE:
            // Button press resets inactivity timer
            LOG_INF("Button press - resetting inactivity timer");
            lastActivityTime = k_uptime_get_32();
            break;

        case PowerState::IN_SESSION:
            // Button press during session - maybe reset data in future?
            LOG_INF("Button press during session (no action)");
            break;
    }
}
