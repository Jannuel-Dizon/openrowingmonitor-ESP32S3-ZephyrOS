#include "fit_profile.h"
#include "fit_crc.h"

typedef FIT_UINT32 FIT_DATE_TIME;

// The SDK expects this structure for file message definitions
typedef struct
{
   FIT_UINT16 num;
   FIT_UINT32 data_file_offset;
} FIT_FILE_MESG;

// Custom structs to handle the inline field definitions required by the SDK
typedef struct {
   FIT_UINT8 reserved_1;
   FIT_UINT8 arch;
   FIT_UINT16 global_mesg_num;
   FIT_UINT8 num_fields;
   FIT_FIELD_DEF fields[FIT_FILE_ID_MESG_DEF_SIZE];
} FIT_FILE_ID_MESG_DEF_STRUCT;

typedef struct {
   FIT_UINT8 reserved_1;
   FIT_UINT8 arch;
   FIT_UINT16 global_mesg_num;
   FIT_UINT8 num_fields;
   FIT_FIELD_DEF fields[FIT_RECORD_MESG_DEF_SIZE];
} FIT_RECORD_MESG_DEF_STRUCT;

static const FIT_FILE_ID_MESG_DEF_STRUCT file_id_mesg_def = {
   0, FIT_ARCH_ENDIAN, FIT_MESG_NUM_FILE_ID, FIT_FILE_ID_MESG_DEF_SIZE,
   {
      { FIT_FILE_ID_FIELD_NUM_TYPE, 1, FIT_BASE_TYPE_ENUM },
      { FIT_FILE_ID_FIELD_NUM_MANUFACTURER, 2, FIT_BASE_TYPE_UINT16 },
      { FIT_FILE_ID_FIELD_NUM_PRODUCT, 2, FIT_BASE_TYPE_UINT16 },
      { FIT_FILE_ID_FIELD_NUM_SERIAL_NUMBER, 4, FIT_BASE_TYPE_UINT32Z },
      { FIT_FILE_ID_FIELD_NUM_TIME_CREATED, 4, FIT_BASE_TYPE_UINT32 },
      { FIT_FILE_ID_FIELD_NUM_NUMBER, 2, FIT_BASE_TYPE_UINT16 }
   }
};

static const FIT_RECORD_MESG_DEF_STRUCT record_mesg_def = {
   0, FIT_ARCH_ENDIAN, FIT_MESG_NUM_RECORD, FIT_RECORD_MESG_DEF_SIZE,
   {
      { FIT_RECORD_FIELD_NUM_TIMESTAMP, 4, FIT_BASE_TYPE_UINT32 },
      { FIT_RECORD_FIELD_NUM_DISTANCE, 4, FIT_BASE_TYPE_UINT32 },
      { FIT_RECORD_FIELD_NUM_CADENCE, 1, FIT_BASE_TYPE_UINT8 },
      { FIT_RECORD_FIELD_NUM_POWER, 2, FIT_BASE_TYPE_UINT16 },
      { FIT_RECORD_FIELD_NUM_HEART_RATE, 1, FIT_BASE_TYPE_UINT8 },
      { FIT_RECORD_FIELD_NUM_STROKE_COUNT, 2, FIT_BASE_TYPE_UINT16 }
   }
};

const FIT_MESG_DEF * const fit_mesg_defs[] = {
   (const FIT_MESG_DEF*)&file_id_mesg_def,
   (const FIT_MESG_DEF*)&record_mesg_def,
   FIT_NULL
};
