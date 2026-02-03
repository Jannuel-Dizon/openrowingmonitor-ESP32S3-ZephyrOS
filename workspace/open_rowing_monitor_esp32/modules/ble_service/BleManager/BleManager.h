#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

class BleManager {
public:
    void init();
    static void startAdvertising();

    // Check if a device is currently connected
    bool isConnected();

    static void onConnected(struct bt_conn *conn, uint8_t err);
    static void onDisconnected(struct bt_conn *conn, uint8_t reason);
    void forEachConnection(void (*func)(struct bt_conn *conn, void *data), void *user_data);
private:
    // Track the active connection
    static struct bt_conn *current_conns[CONFIG_BT_MAX_CONN];
    static struct k_mutex conn_mutex;
    static int active_connections;
    static struct k_work_delayable adv_restart_work;
    static void advRestartHandler(struct k_work *work);
};

#endif // BLE_MANAGER_H
