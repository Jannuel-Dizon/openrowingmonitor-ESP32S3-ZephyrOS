#include "SystemMonitor.h"
#include <cstring>
#include <zephyr/sys/sys_heap.h>

LOG_MODULE_REGISTER(SystemMonitor, LOG_LEVEL_INF);

void SystemMonitor::init() {
    // Initialize thread tracking array
    for (int i = 0; i < MAX_THREADS; i++) {
        threads[i].thread = nullptr;
        threads[i].name = nullptr;
        threads[i].minStackFree = SIZE_MAX;
        threads[i].registered = false;
    }

    // Initialize timer tracking
    for (int i = 0; i < MAX_TIMERS; i++) {
        timers[i].tag = nullptr;
        timers[i].active = false;
    }

    lastUpdateTime = k_uptime_get_32();
    LOG_INF("SystemMonitor initialized");
}

void SystemMonitor::registerThread(struct k_thread *thread, const char *name) {
    if (threadCount >= MAX_THREADS) {
        LOG_WRN("Cannot register thread '%s': max threads reached", name);
        return;
    }

    // Check if already registered
    for (int i = 0; i < threadCount; i++) {
        if (threads[i].thread == thread) {
            LOG_WRN("Thread '%s' already registered", name);
            return;
        }
    }

    threads[threadCount].thread = thread;
    threads[threadCount].name = name;
    threads[threadCount].minStackFree = SIZE_MAX;
    threads[threadCount].registered = true;
    threadCount++;

    LOG_INF("Registered thread '%s' for monitoring", name);
}

size_t SystemMonitor::checkThreadStack(struct k_thread *thread, const char *name) {
    if (thread == nullptr) {
        LOG_ERR("checkThreadStack: null thread pointer");
        return 0;
    }

    size_t unused = 0;
    int ret = k_thread_stack_space_get(thread, &unused);

    if (ret != 0) {
        LOG_ERR("Failed to get stack info for '%s' (error %d)", name, ret);
        return 0;
    }

    // Find this thread in our tracking array to update water mark
    for (int i = 0; i < threadCount; i++) {
        if (threads[i].thread == thread) {
            if (unused < threads[i].minStackFree) {
                threads[i].minStackFree = unused;
                LOG_WRN("[%s] New stack LOW WATER MARK: %u bytes free", name, unused);
            }
            break;
        }
    }

    return unused;
}

void SystemMonitor::checkAllThreads() {
    LOG_INF("=== Thread Stack Report ===");

    for (int i = 0; i < threadCount; i++) {
        if (!threads[i].registered) continue;

        size_t unused = checkThreadStack(threads[i].thread, threads[i].name);

        // Color-coded warnings based on remaining stack
        if (unused < 512) {
            LOG_ERR("[%s] CRITICAL: Only %u bytes free (min: %u)",
                    threads[i].name, unused, threads[i].minStackFree);
        } else if (unused < 1024) {
            LOG_WRN("[%s] WARNING: %u bytes free (min: %u)",
                    threads[i].name, unused, threads[i].minStackFree);
        } else {
            LOG_INF("[%s] OK: %u bytes free (min: %u)",
                    threads[i].name, unused, threads[i].minStackFree);
        }
    }

    LOG_INF("===========================");
}

void SystemMonitor::logMemoryStats() {
    #ifdef CONFIG_SYS_HEAP_RUNTIME_STATS
    // Get the system heap (Zephyr's main heap for k_malloc, etc.)
    extern struct sys_heap _system_heap;

    struct sys_memory_stats stats;
    int ret = sys_heap_runtime_stats_get(&_system_heap, &stats);

    if (ret == 0) {
        LOG_INF("=== Heap Memory Stats ===");
        LOG_INF("  Allocated: %u bytes", stats.allocated_bytes);
        LOG_INF("  Free:      %u bytes", stats.free_bytes);
        LOG_INF("  Max allocated: %u bytes", stats.max_allocated_bytes);
        LOG_INF("=========================");
    } else {
        LOG_WRN("Failed to get heap stats (error %d)", ret);
    }
    #else
    LOG_DBG("Heap runtime stats not enabled (CONFIG_SYS_HEAP_RUNTIME_STATS=n)");
    #endif
}

SystemMonitor::TimerEntry* SystemMonitor::findTimer(const char *tag) {
    // First, try to find existing timer with this tag
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (timers[i].tag != nullptr && strcmp(timers[i].tag, tag) == 0) {
            return &timers[i];
        }
    }

    // If not found, find an empty slot
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (timers[i].tag == nullptr) {
            return &timers[i];
        }
    }

    LOG_WRN("No timer slots available for '%s'", tag);
    return nullptr;
}

void SystemMonitor::startTimer(const char *tag) {
    TimerEntry *timer = findTimer(tag);
    if (timer == nullptr) return;

    timer->tag = tag;
    timer->startTime = k_uptime_get_32();
    timer->active = true;
}

void SystemMonitor::endTimer(const char *tag) {
    TimerEntry *timer = findTimer(tag);
    if (timer == nullptr || !timer->active) {
        LOG_WRN("Timer '%s' was not started", tag);
        return;
    }

    uint32_t elapsed = k_uptime_get_32() - timer->startTime;
    LOG_INF("[PERF] %s: %u ms", tag, elapsed);

    timer->active = false;
}

void SystemMonitor::update(uint32_t interval_ms) {
    uint32_t now = k_uptime_get_32();

    if ((now - lastUpdateTime) >= interval_ms) {
        checkAllThreads();
        logMemoryStats();
        lastUpdateTime = now;
    }
}
