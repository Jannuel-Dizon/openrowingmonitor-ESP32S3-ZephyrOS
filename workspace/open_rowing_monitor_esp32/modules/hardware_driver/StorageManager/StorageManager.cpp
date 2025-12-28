#include "StorageManager.h"
#include <zephyr/logging/log.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/flash_map.h> // <--- REQUIRED for FIXED_PARTITION_ID

LOG_MODULE_REGISTER(storage_manager, LOG_LEVEL_INF);

#define MOUNT_POINT "/lfs"

// Static members
const char* StorageManager::mount_pt = MOUNT_POINT;
const char* StorageManager::log_file_path = "/lfs/session.fit";

StorageManager::StorageManager() {
    // ----------------------------------------------------------------------
    // FIX: Use FIXED_PARTITION_ID with the Node Label
    // This assumes your overlay has: storage_partition: partition@...
    // ----------------------------------------------------------------------
    mp.storage_dev = (void *)FIXED_PARTITION_ID(DT_NODELABEL(storage_partition));

    mp.mnt_point = mount_pt;
    mp.fs_data = &lfs_data;
    mp.type = FS_LITTLEFS;

    // Do not use FS_MOUNT_FLAG_USE_DISK_ACCESS for internal flash
    mp.flags = FS_MOUNT_FLAG_NO_FORMAT;

    // Configure LittleFS
    lfs_data.cfg.read_size = 16;
    lfs_data.cfg.prog_size = 16;
    lfs_data.cfg.cache_size = 64;
    lfs_data.cfg.lookahead_size = 32;
    lfs_data.cfg.block_cycles = 512;
}

// // ... rest of the file (init, appendRecord, etc.) remains the same ...
// #include "StorageManager.h"
// #include <zephyr/logging/log.h>
// #include <zephyr/fs/fs.h>

// LOG_MODULE_REGISTER(storage_manager, LOG_LEVEL_INF);

// // Define partition name from Device Tree (must match app.overlay)
// #define STORAGE_PARTITION_LABEL "storage_partition"
// #define MOUNT_POINT "/lfs"

// // Static members
// const char* StorageManager::mount_pt = MOUNT_POINT;
// const char* StorageManager::log_file_path = "/lfs/session.fit";

// StorageManager::StorageManager() {
//     // 1. USE FLASH_AREA_ID, NOT DEVICE
//     // The cast to (void*) is required by the struct definition
//     mp.storage_dev = (void *)FLASH_AREA_DEVICE(storage_partition);

//     mp.mnt_point = mount_pt;
//     mp.fs_data = &lfs_data;
//     mp.type = FS_LITTLEFS;

//     // 2. REMOVE THIS FLAG
//     // mp.flags = FS_MOUNT_FLAG_USE_DISK_ACCESS;
//     mp.flags = FS_MOUNT_FLAG_NO_FORMAT; // Optional: prevents auto-format on mount (safer)

//     // 3. Configure LittleFS
//     lfs_data.cfg.read_size = 16;
//     lfs_data.cfg.prog_size = 16;
//     lfs_data.cfg.cache_size = 64;
//     lfs_data.cfg.lookahead_size = 32;
//     lfs_data.cfg.block_cycles = 512;
// }

int StorageManager::init() {
    int res = fs_mount(&mp);

    if (res != 0) {
        LOG_WRN("Mount failed, attempting to format... (Error: %d)", res);

        // 3. Use fs_mkfs instead of fs_format
        // fs_mkfs(type, device_ptr, config_ptr, flags)
        res = fs_mkfs(FS_LITTLEFS, (uintptr_t)mp.storage_dev, &lfs_data, 0);

        if (res == 0) {
            LOG_INF("Format successful, remounting...");
            res = fs_mount(&mp);
        }
    }

    if (res == 0) {
        LOG_INF("LittleFS mounted at %s", mount_pt);
        isMounted = true;
    } else {
        LOG_ERR("Failed to init LittleFS (Error: %d)", res);
    }

    return res;
}

bool StorageManager::appendRecord(const std::string& data) {
    if (!isMounted) return false;

    struct fs_file_t file;
    fs_file_t_init(&file);

    // Open in Append + Create mode
    int rc = fs_open(&file, log_file_path, FS_O_CREATE | FS_O_APPEND | FS_O_WRITE);
    if (rc < 0) {
        LOG_ERR("Failed to open file: %d", rc);
        return false;
    }

    rc = fs_write(&file, data.c_str(), data.size());
    fs_close(&file);

    if (rc < 0) {
        LOG_ERR("Failed to write: %d", rc);
        return false;
    }
    return true;
}

// 4. Fixed function name typo (Vol vs Volume)
void StorageManager::listMountedVol() {
    if (!isMounted) return;

    struct fs_dir_t dir;
    fs_dir_t_init(&dir);

    int rc = fs_opendir(&dir, mount_pt);
    if (rc < 0) {
        LOG_ERR("Error opening dir: %d", rc);
        return;
    }

    LOG_INF("Listing files in %s:", mount_pt);
    struct fs_dirent entry;
    while (fs_readdir(&dir, &entry) == 0 && entry.name[0] != 0) {
        LOG_INF("  %c %s (%u bytes)",
            entry.type == FS_DIR_ENTRY_DIR ? 'D' : 'F',
            entry.name,
            entry.size);
    }
    fs_closedir(&dir);
}
