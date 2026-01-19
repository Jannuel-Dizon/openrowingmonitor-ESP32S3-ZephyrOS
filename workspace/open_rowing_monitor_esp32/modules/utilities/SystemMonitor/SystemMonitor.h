#pragma once

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_heap.h>

/**
 * @brief System health monitoring for debugging stack, heap, and thread performance
 *
 * This class provides runtime monitoring of critical system resources:
 * - Stack usage per thread (detects overflows before they crash)
 * - Heap usage (if dynamic allocation is used)
 * - Thread statistics (CPU usage, state)
 * - Performance metrics (loop times, latencies)
 */
class SystemMonitor {
public:
    /**
     * @brief Initialize the monitoring system
     */
    void init();

    /**
     * @brief Register a thread for stack monitoring
     * @param thread Pointer to the thread structure
     * @param name Human-readable name for logging
     */
    void registerThread(struct k_thread *thread, const char *name);

    /**
     * @brief Check all registered threads and log stack usage
     * Call this periodically (e.g., every 30 seconds)
     */
    void checkAllThreads();

    /**
     * @brief Check a specific thread's stack usage
     * @param thread The thread to check
     * @param name Name for logging
     * @return Bytes of stack remaining (0 if error)
     */
    size_t checkThreadStack(struct k_thread *thread, const char *name);

    /**
     * @brief Log system-wide memory statistics
     */
    void logMemoryStats();

    /**
     * @brief Start a performance measurement
     * @param tag Identifier for this measurement
     */
    void startTimer(const char *tag);

    /**
     * @brief End a performance measurement and log the duration
     * @param tag Identifier (must match startTimer call)
     */
    void endTimer(const char *tag);

    /**
     * @brief Periodic update - call this from your main loop
     * @param interval_ms How often to run checks (e.g., 30000 for 30 sec)
     */
    void update(uint32_t interval_ms = 30000);

private:
    struct ThreadInfo {
        struct k_thread *thread;
        const char *name;
        size_t minStackFree;  // Track the lowest we've seen (water mark)
        bool registered;
    };

    static const int MAX_THREADS = 8;
    ThreadInfo threads[MAX_THREADS];
    int threadCount = 0;

    // For performance measurements
    struct TimerEntry {
        const char *tag;
        uint32_t startTime;
        bool active;
    };
    static const int MAX_TIMERS = 4;
    TimerEntry timers[MAX_TIMERS];

    uint32_t lastUpdateTime = 0;

    // Helper to find or create a timer entry
    TimerEntry* findTimer(const char *tag);
};
