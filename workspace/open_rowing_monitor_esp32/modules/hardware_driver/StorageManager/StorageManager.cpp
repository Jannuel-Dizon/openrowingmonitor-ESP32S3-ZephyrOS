#include "StorageManager.h"
#include <zephyr/logging/log.h>
#include <zephyr/drivers/disk.h>

LOG_MODULE_REGISTER(StorageManager, LOG_LEVEL_INF);

// The generic mount point for SD cards in Zephyr is usually mapped to "SD"
const char* StorageManager::disk_mount_pt = "/SD:";
const char* StorageManager::log_file_path = "/SD:/workout.csv";

StorageManager::StorageManager() {
    // Initialize the mount point structure
    mp.type = FS_FATFS;
    mp.fs_data = NULL;
    mp.mnt_point = disk_mount_pt;
}

int StorageManager::init() {
    LOG_INF("Attempting to mount SD card...");

    // 1. Mount the filesystem
    int res = fs_mount(&mp);

    if (res == FR_OK) {
        LOG_INF("SD Card mounted successfully at %s", mp.mnt_point);
        isMounted = true;
        return 0;
    } else {
        LOG_ERR("Error mounting SD Card: %d", res);
        return res;
    }
}

bool StorageManager::appendRecord(const std::string& data) {
    if (!isMounted) return false;

    struct fs_file_t file;
    fs_file_t_init(&file);

    // 1. Open (Create if not exists, Append to end)
    int rc = fs_open(&file, log_file_path, FS_O_CREATE | FS_O_APPEND | FS_O_WRITE);
    if (rc < 0) {
        LOG_ERR("Failed to open file: %d", rc);
        return false;
    }

    // 2. Write
    rc = fs_write(&file, data.c_str(), data.length());
    if (rc < 0) {
        LOG_ERR("Failed to write to file: %d", rc);
        fs_close(&file);
        return false;
    }

    // 3. Write Newline
    fs_write(&file, "\n", 1);

    // 4. Close (This flushes the data to the card)
    fs_close(&file);

    // Optional: Log success (don't do this in production high-speed loops)
    // LOG_DBG("Wrote %d bytes", data.length());

    return true;
}
