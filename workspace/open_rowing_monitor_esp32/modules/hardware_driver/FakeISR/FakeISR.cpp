#include "FakeISR.h"
#include <zephyr/logging/log.h>

#ifndef CONFIG_FAKEISR_PHYSICS_THREAD_STACK_SIZE
#define CONFIG_FAKEISR_PHYSICS_THREAD_STACK_SIZE 4096  // Safe default
#endif

K_THREAD_STACK_DEFINE(physicsThreadStack, CONFIG_FAKEISR_PHYSICS_THREAD_STACK_SIZE);

#define PHYSICS_PRIORITY 5

LOG_MODULE_REGISTER(FakeISR, LOG_LEVEL_INF);

// Stack for the fake ISR thread
K_THREAD_STACK_DEFINE(fake_isr_stack, 2048);


static FakeISR* instance = nullptr;

FakeISR::FakeISR(RowingEngine& engine, bool loop)
    : m_engine(engine),
      m_loop(loop),
      m_is_running(false),
      m_current_index(0) {

          instance = this;

          k_msgq_init(&m_queue, m_queue_buffer, sizeof(uint32_t), IMPULSE_QUEUE_SIZE);

          k_thread_create(&physicsThreadData,
                          physicsThreadStack,
                          K_THREAD_STACK_SIZEOF(physicsThreadStack),
                          physicsThreadEntryPoint,
                          this, NULL, NULL,
                          PHYSICS_PRIORITY,
                          0,
                          K_NO_WAIT);
      }
void FakeISR::start() {
    if (m_is_running) {
        LOG_WRN("Fake ISR already running");
        return;
    }

    LOG_INF("Starting Fake ISR (replaying %zu impulses)", m_data_count);
    m_is_running = true;
    m_current_index = 0;

    k_thread_create(&thread_data,
                    fake_isr_stack,
                    K_THREAD_STACK_SIZEOF(fake_isr_stack),
                    threadEntry,
                    this, NULL, NULL,
                    4,  // Same priority as real ISR handler
                    0,
                    K_NO_WAIT);
}

void FakeISR::stop() {
    if (!m_is_running) {
        return;
    }

    LOG_INF("Stopping Fake ISR");
    m_is_running = false;
    k_thread_join(&thread_data, K_FOREVER);
}

void FakeISR::physicsThreadEntryPoint(void* p1, void* p2, void* p3) {
    FakeISR* self = static_cast<FakeISR*>(p1);
    self->physicsLoop();
}

void FakeISR::physicsLoop() {
    uint32_t deltaCycles;
    LOG_INF("Physics loop thread started");

    while (true) {
        if (k_msgq_get(&m_queue, &deltaCycles, K_FOREVER) == 0) {
            double dt = (double)deltaCycles / (double)sys_clock_hw_cycles_per_sec();
            m_engine.handleRotationImpulse(dt);
        }
    }
}

void FakeISR::threadEntry(void* p1, void* p2, void* p3) {
    FakeISR* self = static_cast<FakeISR*>(p1);
    self->replayLoop();
}

void FakeISR::replayLoop() {
    LOG_INF("Fake ISR thread started");

    uint32_t loop_count = 0;

    while (m_is_running) {
        // Get next dt value
        double dt = m_test_data[m_current_index];

        // Convert to cycles (same as real ISR)
        uint32_t deltaCycles = (uint32_t)(dt * sys_clock_hw_cycles_per_sec());

        // Send to physics thread via message queue
        int ret = k_msgq_put(&m_queue, &deltaCycles, K_NO_WAIT);

        if (ret != 0) {
            LOG_ERR("Queue full! Physics thread can't keep up");
            // In real ISR this would be a serious problem
        }

        // Wait the actual dt time to simulate real timing
        uint32_t sleep_ms = (uint32_t)(dt * 1000.0);
        if (sleep_ms > 0) {
            k_msleep(sleep_ms);
        }

        // Move to next impulse
        m_current_index++;

        // Loop or stop?
        if (m_current_index >= m_data_count) {
            if (m_loop) {
                m_current_index = 0;
                loop_count++;
                LOG_INF("Completed loop %u", loop_count);
            } else {
                LOG_INF("Test data complete");
                m_is_running = false;
                break;
            }
        }
    }

    LOG_INF("Fake ISR thread stopped");
}

size_t FakeISR::getDtCount() {
    return dtCount;
}
