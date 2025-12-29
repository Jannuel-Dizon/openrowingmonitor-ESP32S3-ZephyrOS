#pragma once

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <string>

class StorageManager {
public:
    	StorageManager();
    	int init();
    	bool appendRecord(const std::string& data, const std::string& filename);
    	void listMountedVol();
     	void dumpFile(const std::string& filename);
private:
    	// LittleFS specific structures
    	struct fs_mount_t mp;
    	struct fs_littlefs lfs_data;

     	static const char* mount_pt;

      bool isMounted = false;
};
