#pragma once

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <string>

class StorageManager {
public:
    StorageManager();

    // Mounts the SD card. Returns 0 on success.
    int init();

    // Appends a line of text to "workout.csv"
    // Returns true if successful.
    bool appendRecord(const std::string& data);

private:
    // Zephyr FS structures
    struct fs_mount_t mp;
    static const char* disk_mount_pt;
    static const char* log_file_path;

    bool isMounted = false;
};
