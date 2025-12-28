#include "FITRecorder.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(fit_recorder, LOG_LEVEL_INF);

// FIT Epoch (Seconds between Unix Epoch 1970 and FIT Epoch 1989)
#define FIT_EPOCH_OFFSET 631065600

FitRecorder::FitRecorder(StorageManager* storageMgr)
    : storage(storageMgr), hasSentRecordDef(false) {
}

void FitRecorder::startFile() {
    // 1. Reset State
    hasSentRecordDef = false;

    // 2. Write File ID Message (Required as first message)
    // In a real implementation, you would write the Definition for FileID,
    // then the FileID Data Message here.
    // (Skipped here to focus on logRecord as requested, but logic is identical)
}

void FitRecorder::logRecord(const RowingData& data) {
    if (!storage) return;

    // Buffer for serialization (Max FIT message size is usually small, <255 bytes)
    FIT_UINT8 buffer[255];
    FIT_UINT8 length = 0;

    // --------------------------------------------------------
    // 1. Send Definition Message (Once per file for this type)
    // --------------------------------------------------------
    if (!hasSentRecordDef) {
        // Retrieve the standard definition for the RECORD message (Global ID 20)
        // This comes from your fit_profile.c
        const FIT_MESG_DEF* mesgDef = Fit_GetMesgDef(FIT_MESG_NUM_RECORD);

        if (mesgDef != NULL) {
            // Serialize the definition message
            // Note: usage might vary slightly based on fit_convert version,
            // but usually Fit_ConvertMessageDefinition or similar.
            // If your SDK uses Fit_Convert, we construct the definition header manually or use helper.
            // Standard generic way using fit_convert.c:
            length = Fit_ConvertMessageDefinition(mesgDef, buffer, RECORD_LOCAL_MESG_NUM, FIT_ARCH_ENDIAN_LITTLE);
            writeToStorage(buffer, length);

            hasSentRecordDef = true;
            LOG_INF("Written Record Definition Message");
        } else {
            LOG_ERR("Could not find RECORD message definition in profile!");
            return;
        }
    }

    // --------------------------------------------------------
    // 2. Prepare Data Message
    // --------------------------------------------------------
    FIT_RECORD_MESG record;

    // Initialize struct with invalid values (safety)
    Fit_InitMesg(Fit_GetMesgDef(FIT_MESG_NUM_RECORD), &record);

    // Map RowingData to FIT_RECORD_MESG
    // Timestamp: Convert session time to FIT time
    record.timestamp = getFitTimestamp(data.totalTime);

    // Distance: Meters (float) -> Centimeters (uint32)
    record.distance = (FIT_UINT32)(data.distance * 100.0);

    // Power: Watts (double) -> Watts (uint16)
    record.power = (FIT_UINT16)data.power;

    // Speed: m/s (double) -> mm/s (uint16)
    record.speed = (FIT_UINT16)(data.speed * 1000.0);

    // Cadence: SPM (double) -> SPM (uint8)
    record.cadence = (FIT_UINT8)data.spm;

    // Heart Rate: (0 if not available)
    record.heart_rate = 0;

    // --------------------------------------------------------
    // 3. Serialize Data Message
    // --------------------------------------------------------
    // Fit_ConvertMesg serializes the struct into binary format
    // It returns the number of bytes written to buffer
    length = Fit_ConvertMesg(&record, buffer, RECORD_LOCAL_MESG_NUM);

    // --------------------------------------------------------
    // 4. Write to Storage
    // --------------------------------------------------------
    writeToStorage(buffer, length);
}

void FitRecorder::writeToStorage(const void* data, size_t size) {
    // StorageManager takes std::string, which is safe for binary data
    // IF constructed with specific length.
    std::string binData((const char*)data, size);
    storage->appendRecord(binData);
}

FIT_DATE_TIME FitRecorder::getFitTimestamp(double sessionTimeS) {
    // If you have a Real Time Clock (RTC), use it here.
    // Otherwise, we fake it using a base time (e.g., compile time or fixed date) + session time.
    // Ideally: (CurrentUnixTime - FIT_EPOCH_OFFSET)

    // Placeholder: Start at arbitrary recent date if no RTC
    // (e.g. 2025-01-01 = 1104537600 FIT Time)
    FIT_DATE_TIME baseTime = 1104537600;
    return (FIT_DATE_TIME)(baseTime + (uint32_t)sessionTimeS);
}
