#include "FTMSService.h"

// -----------------------------------------------------------------------------
// 1. Define the GATT Service using Zephyr Macros
// -----------------------------------------------------------------------------

// FTMS Features: Read-only.
// Bit 4 = Rower Supported.
static const uint32_t ftms_feature = 0x00000010;

static ssize_t read_feature(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			    void *buf, uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &ftms_feature,
				 sizeof(ftms_feature));
}

// Configuration Change Callback (CCCD) - Tracks if client subscribed to notifications
static bool notify_enabled = false;
static void rower_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	notify_enabled = (value == BT_GATT_CCC_NOTIFY);
    printk("FTMS Notifications %s\n", notify_enabled ? "ENABLED" : "DISABLED");
}

// Define the Service Layout
BT_GATT_SERVICE_DEFINE(ftms_svc,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_FTMS),

    // Characteristic: Rower Data (0x2AD1) - Notify Only
    BT_GATT_CHARACTERISTIC(BT_UUID_ROWER_DATA,
                           BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_NONE,
                           NULL, NULL, NULL),
    BT_GATT_CCC(rower_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

    // Characteristic: Fitness Machine Feature (0x2ACC) - Read Only
    BT_GATT_CHARACTERISTIC(BT_UUID_FITNESS_MACHINE_FEATURE,
                           BT_GATT_CHRC_READ,
                           BT_GATT_PERM_READ,
                           read_feature, NULL, NULL)
);

// -----------------------------------------------------------------------------
// 2. Class Implementation
// -----------------------------------------------------------------------------

void FTMSService::init() {
    // Zephyr handles GATT initialization automatically via the macro.
    // This function is here if you need to set initial values or debug logs.
    printk("FTMS Service Initialized\n");
}

bool FTMSService::isNotifyEnabled() {
    return notify_enabled;
}

void FTMSService::notifyRowingData(const RowingData& data) {
    if (!notify_enabled) {
        return;
    }

    /* Rower Data Packet Construction (Little Endian)
       Flags (16 bits) determine which fields follow.

       Based on your RowingData struct, we will send:
       - Bit 0: Stroke Rate & Stroke Count
       - Bit 2: Total Distance
       - Bit 3: Instantaneous Pace
       - Bit 4: Average Pace
       - Bit 5: Instantaneous Power
       - Bit 11: Elapsed Time

       Flags = 0000 1000 0011 1101 = 0x083D
    */
    uint16_t flags = 0;
    flags |= BIT(0);  // Stroke Rate / Count
    flags |= BIT(2);  // Distance
    flags |= BIT(3);  // Inst Pace
    flags |= BIT(4);  // Avg Pace
    flags |= BIT(5);  // Power
    flags |= BIT(11); // Elapsed Time

    // Buffer to hold the packet (Max size approx 20-30 bytes)
    uint8_t buffer[30];
    uint8_t cursor = 0;

    // 1. Flags (UINT16)
    sys_put_le16(flags, &buffer[cursor]);
    cursor += 2;

    // 2. Stroke Rate (UINT8) - Unit is 0.5 strokes/min
    // Your data.strokeRate is likely a double (e.g., 24.5).
    // FTMS expects (24.5 * 2) = 49.
    uint8_t stroke_rate = (uint8_t)(data.spm * 2);
    buffer[cursor] = stroke_rate;
    cursor += 1;

    // 3. Stroke Count (UINT16)
    sys_put_le16((uint16_t)data.strokeCount, &buffer[cursor]);
    cursor += 2;

    // 4. Total Distance (UINT24) - Meters
    // sys_put_le24 is a Zephyr helper
    sys_put_le24((uint32_t)data.distance, &buffer[cursor]);
    cursor += 3;

    // 5. Instantaneous Pace (UINT16) - Seconds per 500m
    sys_put_le16((uint16_t)data.instPace, &buffer[cursor]);
    cursor += 2;

    // 6. Average Pace (UINT16) - Seconds per 500m
    sys_put_le16((uint16_t)data.avgPace, &buffer[cursor]);
    cursor += 2;

    // 7. Instantaneous Power (SINT16) - Watts
    sys_put_le16((int16_t)data.strokePower, &buffer[cursor]);
    cursor += 2;

    // 8. Elapsed Time (UINT16) - Seconds
    sys_put_le16((uint16_t)data.totalTime, &buffer[cursor]);
    cursor += 2;

    // Send the Notification
    // &ftms_svc.attrs[1] refers to the Rower Data Characteristic Value attribute
    // Index 0 = Service, Index 1 = Characteristic Def, Index 2 = Characteristic Value
    // Zephyr macros define attributes in a linear array.
    // Usually, the Value is at index 2 (1 is the declaration).
    // Let's rely on finding the attribute by UUID to be safe, or assume index 2.
    // For BT_GATT_SERVICE_DEFINE, index 2 is safe for the first char value.

    int ret = bt_gatt_notify(NULL, &ftms_svc.attrs[2], buffer, cursor);
    if (ret && ret != -ENOTCONN) {
        printk("Failed to notify rower data (err %d)\n", ret);
    }
}
