#pragma once

#include <zephyr/kernel.h>
#include "RowingEngine.h"
#include "TestData.h"


#define IMPULSE_QUEUE_SIZE (CONFIG_FAKEISR_IMPULSE_QUEUE_SIZE * CONFIG_ORM_IMPULSES_PER_REV)

/**
 * @brief Fake ISR for Testing
 *
 * Replays captured dt values by sending them through the message queue,
 * exactly like the real GPIO ISR does. This lets you test the entire
 * system without rowing.
 */
class FakeISR {
public:
    /**
     * @param impulseQueue - The same queue used by GpioTimerService
     * @param loop - If true, continuously loop through data
     */
    FakeISR(RowingEngine& engine, bool loop = true);
    void start();
    void stop();
    size_t getDtCount();
    bool isRunning() const { return m_is_running; }

private:
    RowingEngine& m_engine;
    const double* m_test_data = dtValues;
    size_t m_data_count = dtCount;
    bool m_loop;
    bool m_is_running;
    size_t m_current_index;

    struct k_msgq m_queue;
    char __aligned(8) m_queue_buffer[IMPULSE_QUEUE_SIZE * sizeof(uint32_t)];

    struct k_thread thread_data;
    struct k_thread physicsThreadData;

    void physicsLoop();
    void replayLoop();

    static void physicsThreadEntryPoint(void* p1, void* p2, void* p3);
    static void threadEntry(void* p1, void* p2, void* p3);
};
