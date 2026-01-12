#ifndef FTMS_H
#define FTMS_H

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>

#include "RowingData.h"

// UUID definitions for FTMS
#define BT_UUID_FTMS_VAL             0x1826
#define BT_UUID_FTMS                 BT_UUID_DECLARE_16(BT_UUID_FTMS_VAL)
#define BT_UUID_ROWER_DATA_VAL       0x2AD1
#define BT_UUID_ROWER_DATA           BT_UUID_DECLARE_16(BT_UUID_ROWER_DATA_VAL)
#define BT_UUID_FITNESS_MACHINE_FEATURE_VAL 0x2ACC
#define BT_UUID_FITNESS_MACHINE_FEATURE     BT_UUID_DECLARE_16(BT_UUID_FITNESS_MACHINE_FEATURE_VAL)

class FTMS {
public:
    /**
     * @brief Initialize the FTMS Service (Advertises capabilities)
     * call this once at startup
     */
     void init();

    /**
     * @brief Sends a notification with the latest rowing data
     * @param data The struct from your RowingEngine
     */
    void notifyRowingData(struct bt_conn *conn, const RowingData& data);
};

#endif // FTMS_H
