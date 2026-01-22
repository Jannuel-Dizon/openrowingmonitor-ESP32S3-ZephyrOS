#ifndef ROWER_BRIDGE_H
#define ROWER_BRIDGE_H

#include "RowingEngine.h"
#include "FTMS.h"
#include "BleManager.h"

class RowerBridge {
public:
    RowerBridge(RowingEngine& engine, FTMS& service, BleManager& blemanager);
    void init();
    /**
     * @brief Call this in your main loop to handle data updates
     */
    void update();
    // static void sendToClient(struct bt_conn *conn, void *ptr);
private:
    RowingEngine& m_engine;
    FTMS& m_service;
    BleManager& m_blemanager;

    // Rate limiting: We don't want to spam BLE (max 2-4 Hz is good)
    uint32_t last_update_time = 0;
    const uint32_t UPDATE_INTERVAL_MS = 250; // 4 updates per second
};

#endif // ROWER_BRIDGE_H
