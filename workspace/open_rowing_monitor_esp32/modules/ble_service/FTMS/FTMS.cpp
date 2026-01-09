#include "FTMS.h"

// -----------------------------------------------------------------------------
// 1. Define the GATT Service using Zephyr Macros
// -----------------------------------------------------------------------------

// FTMS Features: Read-only.
// Bit 4 = Rower Supported.
static const uint32_t ftms_feature = 0x0000082D | BIT(4);

static ssize_t read_feature(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			    void *buf, uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &ftms_feature,
				 sizeof(ftms_feature));
}

// Configuration Change Callback (CCCD) - Tracks if client subscribed to notifications
static volatile bool notify_enabled = false;
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

void FTMS::init() {
    // Zephyr handles GATT initialization automatically via the macro.
    // This function is here if you need to set initial values or debug logs.
    printk("FTMS Service Initialized\n");
}

bool FTMS::isNotifyEnabled() {
    return notify_enabled;
}

void FTMS::notifyRowingData(const RowingData& data) {
    if (!notify_enabled) return;

    /* FLAG MAPPING (UINT16):
       Bit 0: 0 (Stroke Rate/Count Present)
       Bit 1: 1 (Avg Stroke Rate Present)
       Bit 2: 1 (Total Distance Present)
       Bit 3: 1 (Inst Pace Present)
       Bit 4: 1 (Avg Pace Present)
       Bit 5: 1 (Inst Power Present)
       Bit 6: 1 (Avg Power Present)
       Bit 11: 1 (Elapsed Time Present)
    */
    uint16_t flags = 0;
    flags |= BIT(1);
    flags |= BIT(2);
    flags |= BIT(3);
    flags |= BIT(4);
    flags |= BIT(5);
    flags |= BIT(6);
    flags |= BIT(11);

    uint8_t buffer[30];
    uint8_t cursor = 0;

    // [1] Flags (UINT16)
    sys_put_le16(flags, &buffer[cursor]);
    cursor += 2;

    // [2] Inst. Stroke Rate (UINT8 - 0.5 unit) & Stroke Count (UINT16)
    // Present because Bit 0 is 0
    buffer[cursor++] = (uint8_t)(data.spm * 2.0);
    sys_put_le16((uint16_t)data.strokeCount, &buffer[cursor]);
    cursor += 2;

    // [3] Average Stroke Rate (UINT8 - 0.5 unit)
    // Present because Bit 1 is 1
    buffer[cursor++] = (uint8_t)(data.avgSpm * 2.0);

    // [4] Total Distance (UINT24 - Meters)
    // Present because Bit 2 is 1
    sys_put_le24((uint32_t)data.distance, &buffer[cursor]);
    cursor += 3;

    // [5] Instantaneous Pace (UINT16 - Seconds per 500m)
    // Present because Bit 3 is 1
    uint32_t rawPace = (uint32_t)(500.0 / data.speed);
    uint16_t instPace = (rawPace > 65535) ? 65535 : (uint16_t)rawPace;
    sys_put_le16(instPace, &buffer[cursor]);
    cursor += 2;

    // [6] Average Pace (UINT16 - Seconds per 500m)
    // Present because Bit 4 is 1
    uint16_t avgPace = (data.avgSpeed > 0.1) ? (uint16_t)(500.0 / data.avgSpeed) : 0;
    sys_put_le16(avgPace, &buffer[cursor]);
    cursor += 2;

    // [7] Instantaneous Power (SINT16 - Watts)
    // Present because Bit 5 is 1
    sys_put_le16((int16_t)data.power, &buffer[cursor]);
    cursor += 2;

    // [8] Average Power (SINT16 - Watts)
    // Present because Bit 6 is 1
    sys_put_le16((int16_t)data.avgPower, &buffer[cursor]);
    cursor += 2;

    // [9] Elapsed Time (UINT16 - Seconds)
    // Present because Bit 11 is 1
    sys_put_le16((uint16_t)data.totalTime, &buffer[cursor]);
    cursor += 2;

    // Total buffer size used will be 18 bytes
    bt_gatt_notify(NULL, &ftms_svc.attrs[2], buffer, cursor);
}


}
