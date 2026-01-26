#include "BleManager.h"
#include "FTMS.h" // To get UUID definitions

struct bt_conn *BleManager::current_conns[CONFIG_BT_MAX_CONN] = {nullptr}; // Initialize array
int BleManager::active_connections = 0;

LOG_MODULE_REGISTER(BleManager, LOG_LEVEL_INF);
K_MUTEX_DEFINE(BleManager::conn_mutex);
// GAP Connection Callbacks
BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = BleManager::onConnected,
    .disconnected = BleManager::onDisconnected,
};

// -----------------------------------------------------------------------------
// ADVERTISING DATA
// -----------------------------------------------------------------------------
// 1. Flags: General Discoverable
// 2. UUIDs: We MUST advertise the FTMS Service UUID (0x1826)
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

void BleManager::init() {
    int err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)\n", err);// change to LOG_ERR
        return;
    }
    LOG_INF("Bluetooth Initialized");
    active_connections = 0;
    startAdvertising();
}

void BleManager::startAdvertising() {
    // int err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    int err = bt_le_adv_start(BT_LE_ADV_PARAM(
            BT_LE_ADV_OPT_CONN,
            BT_GAP_ADV_FAST_INT_MIN_2,
            BT_GAP_ADV_FAST_INT_MAX_2,
            NULL),
        ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err >= 0 || err == -120) {
        LOG_INF("Advertising successfully started");
        return;
    }
    LOG_ERR("Advertising failed to start (err %d)\n", err);
}

bool BleManager::isConnected() {
    return (active_connections > 0);
}

void BleManager::onConnected(struct bt_conn *conn, uint8_t err) {
    if (err) {
        LOG_ERR("Connection failed (err 0x%02x)", err);
    } else {
        LOG_INF("Connected");
        k_mutex_lock(&conn_mutex, K_FOREVER); // PROTECT THIS
        for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
            if (current_conns[i] == nullptr) {
                current_conns[i] = bt_conn_ref(conn);
                active_connections++;
                LOG_INF("Connected (Slot %d, Total %d)", i, active_connections);
                break;
            }
        }
        k_mutex_unlock(&conn_mutex);
        if (active_connections < CONFIG_BT_MAX_CONN) {
            startAdvertising();
        }
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
            if (active_connections < 0) active_connections = 0;
            break;
        }
    }
    k_mutex_unlock(&conn_mutex);
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
