#include "RowerBridge.h"
#include <zephyr/kernel.h> // For k_uptime_get()

LOG_MODULE_REGISTER(RowerBridge, LOG_LEVEL_INF);

struct Context {
    FTMS* tmp_service;
    RowingData* tmp_data;
};

RowerBridge::RowerBridge(RowingEngine& engine, FTMS& service, BleManager& blemanager)
    : m_engine(engine), m_service(service), m_blemanager(blemanager) {
    }
void RowerBridge::init() {
    LOG_INF("RowerBridge Initialized");
}
void RowerBridge::update() {
    // 1. Check if enough time has passed (Rate Limiting)
    uint32_t now = k_uptime_get_32();
    if ((now - last_update_time) < UPDATE_INTERVAL_MS) {
        return;
    }
    last_update_time = now;

    // 2. Get Fresh Data from Physics Engine
    RowingData data = m_engine.getData();
    // m_engine.printData();
    Context ctx = {&m_service, &data};
    // 3. Send data to all clients
    m_blemanager.forEachConnection([](struct bt_conn *conn, void *ptr) {
        Context *c = static_cast<Context*>(ptr);
        c->tmp_service->notifyRowingData(conn, *(c->tmp_data));
    }, &ctx);
}
