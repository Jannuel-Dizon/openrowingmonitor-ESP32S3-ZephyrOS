#ifndef FIT_PROFILE_H
#define FIT_PROFILE_H

#include "fit_config.h"
#include "fit.h"

#if defined(__cplusplus)
extern "C" {
#endif

// Message Numbers
#define FIT_MESG_NUM_FILE_ID     ((FIT_MESG_NUM)0)
#define FIT_MESG_NUM_SESSION     ((FIT_MESG_NUM)18)
#define FIT_MESG_NUM_RECORD      ((FIT_MESG_NUM)20)

// Field Counts
#define FIT_FILE_ID_MESG_DEF_SIZE   6
#define FIT_SESSION_MESG_DEF_SIZE   15
#define FIT_RECORD_MESG_DEF_SIZE    6

// Field Numbers
#define FIT_FILE_ID_FIELD_NUM_TYPE              ((FIT_UINT8)0)
#define FIT_FILE_ID_FIELD_NUM_MANUFACTURER      ((FIT_UINT8)1)
#define FIT_FILE_ID_FIELD_NUM_PRODUCT           ((FIT_UINT8)2)
#define FIT_FILE_ID_FIELD_NUM_SERIAL_NUMBER     ((FIT_UINT8)3)
#define FIT_FILE_ID_FIELD_NUM_TIME_CREATED      ((FIT_UINT8)4)
#define FIT_FILE_ID_FIELD_NUM_NUMBER            ((FIT_UINT8)5)

#define FIT_RECORD_FIELD_NUM_TIMESTAMP          ((FIT_UINT8)253)
#define FIT_RECORD_FIELD_NUM_DISTANCE           ((FIT_UINT8)5)
#define FIT_RECORD_FIELD_NUM_CADENCE            ((FIT_UINT8)4)
#define FIT_RECORD_FIELD_NUM_POWER              ((FIT_UINT8)7)
#define FIT_RECORD_FIELD_NUM_HEART_RATE         ((FIT_UINT8)3)
#define FIT_RECORD_FIELD_NUM_STROKE_COUNT       ((FIT_UINT8)12)

// Global declaration
extern const FIT_MESG_DEF * const fit_mesg_defs[];

#if defined(__cplusplus)
}
#endif

#endif // FIT_PROFILE_H
