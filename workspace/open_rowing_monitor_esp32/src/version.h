#pragma once

// Open Rowing Monitor - ESP32 Version
#define ORM_VERSION_MAJOR 1
#define ORM_VERSION_MINOR 0
#define ORM_VERSION_PATCH 0

#define ORM_VERSION_STRING "1.0.0"
#define ORM_BUILD_DATE __DATE__
#define ORM_BUILD_TIME __TIME__

// Hardware compatibility
#define ORM_HARDWARE_ESP32S3
#define ORM_MIN_FLASH_SIZE_MB 8

// Feature flags
#define ORM_FEATURE_BLE_FTMS
#define ORM_FEATURE_AUTO_DRAG
#define ORM_FEATURE_MULTI_MAGNET

// Build configuration
#ifdef CONFIG_DEBUG
#define ORM_BUILD_TYPE "Debug"
#else
#define ORM_BUILD_TYPE "Release"
#endif
