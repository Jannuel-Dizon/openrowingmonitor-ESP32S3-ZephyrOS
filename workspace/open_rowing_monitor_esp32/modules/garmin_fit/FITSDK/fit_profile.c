#include "fit_profile.h"

// --- FILE ID FIELDS ---
static const FIT_FIELD_DEF file_id_fields[] = {
   {  3, 4 }, // serial_number
   {  4, 4 }, // time_created
   {  1, 2 }, // manufacturer
   {  2, 2 }, // product
   {  5, 2 }, // number
   {  0, 1 }  // type
};

// --- SESSION FIELDS ---
static const FIT_FIELD_DEF session_fields[] = {
   { 253, 4 }, // timestamp
   {   2, 4 }, // start_time
   {   7, 4 }, // total_elapsed_time
   {   8, 4 }, // total_timer_time
   {   9, 4 }, // total_distance
   {  16, 4 }, // total_cycles (mapped to total_strokes)
   {  11, 2 }, // total_calories
   {  14, 2 }, // avg_speed
   {  15, 2 }, // max_speed
   {  20, 2 }, // avg_power
   {  21, 2 }, // max_power
   {  18, 1 }, // avg_cadence
   {  19, 1 }, // max_cadence
   {   5, 1 }, // sport
   {   6, 1 }  // sub_sport
};

// --- RECORD FIELDS ---
static const FIT_FIELD_DEF record_fields[] = {
   { 253, 4 }, // timestamp
   {   5, 4 }, // distance
   {   7, 2 }, // power
   {   6, 2 }, // speed
   {   4, 1 }, // cadence (mapped to SPM)
   {   3, 1 }  // heart_rate
};

// --- ACTIVITY FIELDS ---
static const FIT_FIELD_DEF activity_fields[] = {
   { 253, 4 }, // timestamp
   {   0, 4 }, // total_timer_time
   {   5, 4 }, // local_timestamp
   { 254, 4 }, // timezone_offset
   {   3, 1 }, // type
   {   1, 1 }, // event
   {   2, 1 }  // event_type
};

// --- MASTER LOOKUP TABLE ---
// This connects the IDs to the definitions
const FIT_CONST_MESG_DEF fit_mesg_defs[] = {
   { 0, FIT_MESG_NUM_FILE_ID,  FIT_FILE_ID_MESG_DEF_SIZE,  file_id_fields,  0 },
   { 0, FIT_MESG_NUM_SESSION,  FIT_SESSION_MESG_DEF_SIZE,  session_fields,  0 },
   { 0, FIT_MESG_NUM_RECORD,   FIT_RECORD_MESG_DEF_SIZE,   record_fields,   0 },
   { 0, FIT_MESG_NUM_ACTIVITY, FIT_ACTIVITY_MESG_DEF_SIZE, activity_fields, 0 },
   { 0, FIT_MESG_NUM_INVALID,  0,                          FIT_NULL,        0 }
};
