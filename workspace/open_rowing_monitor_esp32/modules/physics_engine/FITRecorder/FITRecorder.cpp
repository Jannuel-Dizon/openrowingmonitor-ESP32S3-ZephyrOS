#include "FITRecorder.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(fit_recorder, LOG_LEVEL_INF);

// FIT Epoch (Seconds between Unix Epoch 1970 and FIT Epoch 1989)
#define FIT_EPOCH_OFFSET 631065600

FitRecorder::FitRecorder(StorageManager* storageMgr)
    : storage(storageMgr), hasSentRecordDef(false) {
}

void FitRecorder::startFile() {
    hasSentRecordDef = false;
    // (Optional) Write File Header here if not handled elsewhere
}

void FitRecorder::logRecord(const RowingData& data) {
    if (!storage) return;

    FIT_UINT8 buffer[128]; // Buffer for serialized message
    FIT_UINT8 length = 0;

    // -----------------------------------------------------------
    // 1. Send Definition Message (Once)
    // -----------------------------------------------------------
    if (!hasSentRecordDef) {
        const FIT_MESG_DEF* mesgDef = Fit_GetMesgDef(FIT_MESG_NUM_RECORD);
        if (mesgDef) {
            // Manually serialize Definition Message
            // Header: (MsgNum & 0x0F) | 0x40 (Definition Bit)
            buffer[length++] = (RECORD_LOCAL_MESG_NUM & 0x0F) | 0x40;
            buffer[length++] = 0; // Reserved
            buffer[length++] = FIT_ARCH_ENDIAN_LITTLE;

            // Global Message Number (uint16)
            FIT_UINT16 globalNum = mesgDef->global_mesg_num;
            memcpy(&buffer[length], &globalNum, 2); length+=2;

            // Num Fields
            buffer[length++] = mesgDef->num_fields;

            // Fields
            for(int i=0; i<mesgDef->num_fields; i++) {
                buffer[length++] = mesgDef->fields[i].num;
                buffer[length++] = mesgDef->fields[i].size;
                buffer[length++] = mesgDef->fields[i].type;
            }

            writeToStorage(buffer, length);
            hasSentRecordDef = true;
        }
    }

    // -----------------------------------------------------------
    // 2. Prepare & Serialize Data Message
    // -----------------------------------------------------------
    length = 0;
    FIT_RECORD_MESG record;
    Fit_InitMesg(Fit_GetMesgDef(FIT_MESG_NUM_RECORD), &record);

    // Populate Data
    record.timestamp = getFitTimestamp(data.totalTime);
    record.distance = (FIT_UINT32)(data.distance * 100.0);
    record.power = (FIT_UINT16)data.power;
    record.speed = (FIT_UINT16)(data.speed * 1000.0);
    record.cadence = (FIT_UINT8)data.spm;

    // Manually serialize Data Message based on Profile Definition
    // (A generic SDK function for this is often massive, so we do it simply here)

    // Header: (LocalNum & 0x0F) (Normal Header)
    buffer[length++] = (RECORD_LOCAL_MESG_NUM & 0x0F);

    // We must write fields IN THE ORDER defined in fit_profile.c
    // Check your fit_profile.c -> record_fields array order!
    // Assuming order: timestamp (4), distance (4), speed (2), power (2), cadence (1)

    // Note: This relies on Little Endian CPU (ESP32 is LE)
    memcpy(&buffer[length], &record.timestamp, 4); length+=4;
    memcpy(&buffer[length], &record.distance, 4); length+=4;
    memcpy(&buffer[length], &record.speed, 2); length+=2;
    memcpy(&buffer[length], &record.power, 2); length+=2;
    memcpy(&buffer[length], &record.cadence, 1); length+=1;
    // heart_rate (1) if in profile
    // memcpy(&buffer[length], &record.heart_rate, 1); length+=1;

    writeToStorage(buffer, length);
}

void FitRecorder::writeToStorage(const void* data, size_t size) {
    std::string binData((const char*)data, size);
    storage->appendRecord(binData);
}

FIT_DATE_TIME FitRecorder::getFitTimestamp(double sessionTimeS) {
    return (FIT_DATE_TIME)(1104537600 + (uint32_t)sessionTimeS);
}
