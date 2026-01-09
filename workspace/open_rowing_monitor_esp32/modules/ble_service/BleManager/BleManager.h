#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/printk.h>

class BleManager {
public:
    void init();
    void startAdvertising();

    // Check if a device is currently connected
    bool isConnected();

private:
    static void onConnected(struct bt_conn *conn, uint8_t err);
    static void onDisconnected(struct bt_conn *conn, uint8_t reason);

    // Track the active connection
    static struct bt_conn *current_conn;
};

#endif // BLE_MANAGER_H
