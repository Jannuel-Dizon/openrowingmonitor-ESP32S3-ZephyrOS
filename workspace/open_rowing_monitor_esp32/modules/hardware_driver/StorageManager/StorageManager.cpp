#include "StorageManager.h"
#include <zephyr/logging/log.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/flash_map.h>
#include <cstring>

LOG_MODULE_REGISTER(storage_manager, LOG_LEVEL_INF);

#define MOUNT_POINT "/lfs"

const char* StorageManager::mount_pt = MOUNT_POINT;

StorageManager::StorageManager() {
	memset(&mp, 0, sizeof(mp));
	memset(&lfs_data, 0, sizeof(lfs_data));
	mp.storage_dev = (void *)FIXED_PARTITION_ID(storage_partition);

	mp.mnt_point = mount_pt;
	mp.fs_data = &lfs_data;
	mp.type = FS_LITTLEFS;
	mp.flags = FS_MOUNT_FLAG_NO_FORMAT;

	// LittleFS Config
	lfs_data.cfg.read_size = 16;
	lfs_data.cfg.prog_size = 16;
	lfs_data.cfg.cache_size = 64;
	lfs_data.cfg.lookahead_size = 32;
	lfs_data.cfg.block_cycles = 512;
}

int StorageManager::init() {
	// 1. Try to mount
	int res = fs_mount(&mp);

	// 2. If mount failed, try to format
	if (res != 0) {
		LOG_WRN("Mount failed (Error: %d), attempting to format...", res);

		// Unmount just in case (optional, but safe)
		fs_unmount(&mp);

		// Format the filesystem
		res = fs_mkfs(FS_LITTLEFS, (uintptr_t)mp.storage_dev, &lfs_data, 0);

		if (res == 0) {
        		LOG_INF("Format successful, remounting...");
         		res = fs_mount(&mp);
        	} else {
        		LOG_ERR("Format failed (Error: %d)", res);
         		return res;
        	}
	}

	if (res == 0) {
        	LOG_INF("LittleFS mounted at %s", mp.mnt_point);
         	isMounted = true;
          	listMountedVol(); // Verify it shows up now
    	} else {
        	LOG_ERR("Final mount failed (Error: %d)", res);
     	}

	return res;
}

bool StorageManager::appendRecord(const std::string& data, const std::string& filename) {
    	if (!isMounted) return false;

     	struct fs_file_t file;
      	fs_file_t_init(&file);

       std::string fullPath = std::string(mount_pt) + "/" + filename;
       int rc = fs_open(&file, fullPath.c_str(), FS_O_CREATE | FS_O_WRITE);
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

void StorageManager::dumpFile(const std::string& filename) {
    struct fs_file_t file;
    fs_file_t_init(&file);

    // Construct the full path (e.g., /lfs/test.csv)
    // Adjust "/lfs/" if your mount point is named differently
    std::string fullPath = std::string(mount_pt) + "/" + filename;

    int rc = fs_open(&file, fullPath.c_str(), FS_O_READ);
    if (rc < 0) {
        LOG_ERR("Failed to open %s for reading (%d)", fullPath.c_str(), rc);
        return;
    }

    LOG_INF("--- Start of %s ---", filename.c_str());

    char buffer[128];
    // fs_gets reads until \n or buffer size limit
    while (fs_read(&file, buffer, sizeof(buffer))) {
        // Use printk for raw output to avoid double newlines from LOG_INF
        LOG_INF("%s", buffer);
    }

    LOG_INF("--- End of %s ---", filename.c_str());

    fs_close(&file);
}

void StorageManager::listMountedVol() {
	int index = 0;
	const char *name;
	int rc;

	LOG_INF("--- Checking Mounted File Systems ---");

	// Loop until fs_readmount returns -ENOENT (Entry Not Found)
	while ((rc = fs_readmount(&index, &name)) == 0) {
		// Note: 'index' is incremented inside fs_readmount upon success
		LOG_INF("Found mount point: %s", name);
	}

	if (index == 0) {
		LOG_INF("No file systems mounted.");
	} else {
		LOG_INF("Total file systems found: %d", index);
	}
	LOG_INF("-------------------------------------");
}
