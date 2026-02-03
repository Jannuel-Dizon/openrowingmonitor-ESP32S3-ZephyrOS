#include "BleManager.h"
#include "FTMS.h" // To get UUID definitions

struct bt_conn *BleManager::current_conns[CONFIG_BT_MAX_CONN] = {nullptr};
int BleManager::active_connections = 0;
struct k_event *BleManager::state_change_event = nullptr;

LOG_MODULE_REGISTER(BleManager, LOG_LEVEL_INF);
K_MUTEX_DEFINE(BleManager::conn_mutex);

// GAP Connection Callbacks
BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = BleManager::onConnected,
    .disconnected = BleManager::onDisconnected,
};

// Define the work item
K_WORK_DELAYABLE_DEFINE(BleManager::adv_restart_work, BleManager::advRestartHandler);

// -----------------------------------------------------------------------------
// ADVERTISING DATA
// -----------------------------------------------------------------------------
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL,
                  BT_UUID_16_ENCODE(BT_UUID_FTMS_VAL)), // 0x1826
    BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, BT_UUID_16_ENCODE(CONFIG_BT_DEVICE_APPEARANCE))
};

// Scan Response: Show the Device Name
static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

void BleManager::init(struct k_event* main_event_group) {
    int err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return;
    }
    LOG_INF("Bluetooth Initialized");
    active_connections = 0;

    if (main_event_group == nullptr) {
        LOG_ERR("Event was not registered");
        return;
    }

    state_change_event = main_event_group;

    // Start advertising directly (no work queue needed here)
    // The BT stack is ready after bt_enable() returns successfully
    startAdvertising();
}

void BleManager::startAdvertising() {
    // Check if we're already at max connections
    k_mutex_lock(&conn_mutex, K_FOREVER);
    int conn_count = active_connections;
    k_mutex_unlock(&conn_mutex);

    if (conn_count >= CONFIG_BT_MAX_CONN) {
        LOG_DBG("Max connections reached, not advertising");
        return;
    }

    int err = bt_le_adv_start(BT_LE_ADV_PARAM(
            BT_LE_ADV_OPT_CONN,
            BT_GAP_ADV_FAST_INT_MIN_2,
            BT_GAP_ADV_FAST_INT_MAX_2,
            NULL),
        ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

    if (err == 0) {
        LOG_INF("Advertising successfully started");
    } else if (err == -EALREADY) {
        LOG_DBG("Advertising already active");
    } else {
        LOG_ERR("Advertising failed to start (err %d)", err);
    }
}

bool BleManager::isConnected() {
    k_mutex_lock(&conn_mutex, K_FOREVER);
    bool connected = (active_connections > 0);
    k_mutex_unlock(&conn_mutex);
    return connected;
}

void BleManager::onConnected(struct bt_conn *conn, uint8_t err) {
    if (err) {
        LOG_ERR("Connection failed (err 0x%02x)", err);
        return;
    }

    k_mutex_lock(&conn_mutex, K_FOREVER);

    bool slot_found = false;
    for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
        if (current_conns[i] == nullptr) {
            current_conns[i] = bt_conn_ref(conn);
            if(active_connections == 0 && state_change_event != nullptr) {
                LOG_INF("First connection");
                k_event_post(state_change_event, BIT(0));
            }
            active_connections++;
            slot_found = true;
            LOG_INF("Connected (Slot %d, Total %d)", i, active_connections);
            break;
        }
    }

    int current_conn_count = active_connections;
    k_mutex_unlock(&conn_mutex);

    if (!slot_found) {
        LOG_WRN("No free connection slots!");
        return;
    }

    LOG_INF("Connected");

    // Use work queue ONLY for reconnecting after a connection
    // This prevents race conditions with the BLE stack
    if (current_conn_count < CONFIG_BT_MAX_CONN) {
        // Cancel any pending restart work first
        k_work_cancel_delayable(&adv_restart_work);
        // Schedule restart after 200ms to let connection stabilize
        k_work_schedule(&adv_restart_work, K_MSEC(200));
    }
}

void BleManager::onDisconnected(struct bt_conn *conn, uint8_t reason) {
    LOG_INF("Disconnected (reason 0x%02x)", reason);

    k_mutex_lock(&conn_mutex, K_FOREVER);
    for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
        if (current_conns[i] == conn) {
            bt_conn_unref(current_conns[i]);
            current_conns[i] = nullptr;
            active_connections--;
            if(active_connections == 0 && state_change_event != nullptr) {
                LOG_INF("Last connection lost");
                k_event_post(state_change_event, BIT(1));
            }
            if (active_connections < 0) active_connections = 0;
            LOG_INF("Slot %d freed, Total %d", i, active_connections);
            break;
        }
    }
    k_mutex_unlock(&conn_mutex);

    // Cancel any pending restart work
    k_work_cancel_delayable(&adv_restart_work);

    // Restart advertising immediately on disconnect (safe to do directly)
    startAdvertising();
}

void BleManager::advRestartHandler(struct k_work *work) {
    LOG_DBG("Work handler: Restarting advertising");

    // Stop any existing advertising first
    int err = bt_le_adv_stop();
    if (err && err != -EALREADY) {
        LOG_WRN("Failed to stop advertising (err %d)", err);
    }

    // Small delay to ensure clean state
    k_msleep(10);

    // Restart advertising
    startAdvertising();
}

void BleManager::forEachConnection(void (*func)(struct bt_conn *conn, void *ptr), void *user_data) {
    struct bt_conn *safe_conns[CONFIG_BT_MAX_CONN];
    int count = 0;

    // Prevents lock out because of the MUTEX
    k_mutex_lock(&conn_mutex, K_FOREVER);
    for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
        if (current_conns[i]) {
            safe_conns[count++] = bt_conn_ref(current_conns[i]);
        }
    }
    k_mutex_unlock(&conn_mutex);

    for (int i = 0; i < count; i++) {
        func(safe_conns[i], user_data);
        bt_conn_unref(safe_conns[i]);
    }
}
