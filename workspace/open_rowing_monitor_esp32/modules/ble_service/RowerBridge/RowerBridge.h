#ifndef ROWER_BRIDGE_H
#define ROWER_BRIDGE_H

#include "RowingEngine.h"
#include "FTMS.h"

class RowerBridge {
public:
    RowerBridge(RowingEngine& engine, FTMS& service);

    /**
     * @brief Call this in your main loop to handle data updates
     */
    void update();

private:
    RowingEngine& m_engine;
    FTMS& m_service;

    // Rate limiting: We don't want to spam BLE (max 2-4 Hz is good)
    uint32_t last_update_time = 0;
    const uint32_t UPDATE_INTERVAL_MS = 500; // 2 updates per second
};

#endif // ROWER_BRIDGE_H
