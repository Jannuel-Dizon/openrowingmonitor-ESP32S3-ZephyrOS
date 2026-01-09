#include "BleManager.h"
#include "FTMS.h" // To get UUID definitions

struct bt_conn *BleManager::current_conn = nullptr;

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
                  BT_UUID_16_ENCODE(BT_UUID_FTMS_VAL)) // 0x1826
};

// Scan Response: Show the Device Name
static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

void BleManager::init() {
    int err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }
    printk("Bluetooth initialized\n");
    startAdvertising();
}

void BleManager::startAdvertising() {
    int err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }
    printk("Advertising successfully started\n");
}

bool BleManager::isConnected() {
    return (current_conn != nullptr);
}

void BleManager::onConnected(struct bt_conn *conn, uint8_t err) {
    if (err) {
        printk("Connection failed (err 0x%02x)\n", err);
    } else {
        printk("Connected\n");
        current_conn = bt_conn_ref(conn);
    }
}

void BleManager::onDisconnected(struct bt_conn *conn, uint8_t reason) {
    printk("Disconnected (reason 0x%02x)\n", reason);
    if (current_conn) {
        bt_conn_unref(current_conn);
        current_conn = nullptr;
    }
}
