#ifndef FIT_PROFILE_H
#define FIT_PROFILE_H

#include "fit.h"

#if defined(__cplusplus)
extern "C" {
#endif

// Profile Version (Matches SDK 21.188)
#define FIT_PROFILE_VERSION     ((FIT_UINT16)21188)

// Enum Definitions for Rowing
#define FIT_SPORT_ROWING            ((FIT_ENUM)15)
#define FIT_SUB_SPORT_INDOOR_ROWING ((FIT_ENUM)14)

// Message Global IDs
#define FIT_MESG_NUM_FILE_ID    ((FIT_MESG_NUM)0)
#define FIT_MESG_NUM_SESSION    ((FIT_MESG_NUM)18)
#define FIT_MESG_NUM_RECORD     ((FIT_MESG_NUM)20)
#define FIT_MESG_NUM_ACTIVITY   ((FIT_MESG_NUM)34)

// --------------------------------------------------------
// MESSAGE DEFINITIONS (STRUCTS)
// --------------------------------------------------------

// 1. File ID (Header)
typedef struct {
   FIT_UINT32 serial_number;
   FIT_DATE_TIME time_created;
   FIT_UINT16 manufacturer;
   FIT_UINT16 product;
   FIT_UINT16 number;
   FIT_ENUM type;
} FIT_FILE_ID_MESG;

// 2. Session (Summary of the workout)
typedef struct {
   FIT_DATE_TIME timestamp;       // s (since 1989)
   FIT_DATE_TIME start_time;      // s
   FIT_UINT32 total_elapsed_time; // 1000 * s (ms)
   FIT_UINT32 total_timer_time;   // 1000 * s (ms)
   FIT_UINT32 total_distance;     // 100 * m (cm)
   FIT_UINT32 total_strokes;      // counts (Field 16: total_cycles)
   FIT_UINT16 total_calories;     // kcal
   FIT_UINT16 avg_speed;          // 1000 * m/s (mm/s)
   FIT_UINT16 max_speed;          // 1000 * m/s (mm/s)
   FIT_UINT16 avg_power;          // watts
   FIT_UINT16 max_power;          // watts
   FIT_UINT8 avg_cadence;         // spm
   FIT_UINT8 max_cadence;         // spm
   FIT_ENUM sport;                // 15 = Rowing
   FIT_ENUM sub_sport;            // 14 = Indoor Rowing
} FIT_SESSION_MESG;

// 3. Record (The live data points - 1Hz)
typedef struct {
   FIT_DATE_TIME timestamp;      // s
   FIT_UINT32 distance;          // 100 * m (cm)
   FIT_UINT16 power;             // watts
   FIT_UINT16 speed;             // 1000 * m/s (mm/s)
   FIT_UINT8 cadence;            // spm (Field 4)
   FIT_UINT8 heart_rate;         // bpm
} FIT_RECORD_MESG;

// 4. Activity (Summary of the file)
typedef struct {
   FIT_DATE_TIME timestamp;
   FIT_UINT32 total_timer_time;  // 1000 * s (ms)
   FIT_UINT32 local_timestamp;
   FIT_INT32 timezone_offset;    // s
   FIT_ENUM type;                // 4 = Activity
   FIT_ENUM event;               // 1 = Activity
   FIT_ENUM event_type;          // 1 = Stop
} FIT_ACTIVITY_MESG;

// Message Sizes (Required for buffer allocation)
#define FIT_FILE_ID_MESG_DEF_SIZE   6
#define FIT_SESSION_MESG_DEF_SIZE   15
#define FIT_RECORD_MESG_DEF_SIZE    6
#define FIT_ACTIVITY_MESG_DEF_SIZE  7

extern const FIT_CONST_MESG_DEF fit_mesg_defs[];

#if defined(__cplusplus)
}
#endif

#endif // FIT_PROFILE_H
