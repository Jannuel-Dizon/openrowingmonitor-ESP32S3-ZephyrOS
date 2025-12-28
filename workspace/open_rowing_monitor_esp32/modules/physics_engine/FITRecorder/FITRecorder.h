#pragma once

#include <string>
#include <vector>
#include "RowingData.h"
#include "StorageManager.h"

// Include the C-based FIT SDK headers
extern "C" {
    #include "fit_profile.h"
    #include "fit_convert.h"
    #include "fit_crc.h"
}

class FitRecorder {
public:
    explicit FitRecorder(StorageManager* storageMgr);

    // Initialize the file (Write FileID)
    void startFile();

    // Log a single 1Hz record
    void logRecord(const RowingData& data);

private:
    StorageManager* storage;
    bool hasSentRecordDef;

    // The "Local Message Number" we assign to Record Messages (0-15)
    // We use 0 for Record to keep it simple.
    static const FIT_UINT8 RECORD_LOCAL_MESG_NUM = 0;

    // Helper to write raw bytes to storage
    void writeToStorage(const void* data, size_t size);

    // Helper to get current FIT timestamp (Seconds since Dec 31, 1989)
    FIT_DATE_TIME getFitTimestamp(double sessionTimeS);
};
