cmake_minimum_required(VERSION 3.22.0)

set(ZEPHYR_EXTRA_MODULES
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rowing_core/RowingData
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rowing_core/RowingSettings
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/physics_engine/RowingEngine
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/physics_engine/MovingFlankDetector
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/physics_engine/MovingAverager
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/hardware_driver/GpioTimerService
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/hardware_driver/FakeISR
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/hardware_driver/InputTimerService
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/ble_service/BleManager
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/ble_service/FTMS
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/ble_service/RowerBridge
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/utilities/SystemMonitor
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(ESP32_ORM)

target_sources(app PRIVATE src/main.cpp)

# 2. Add Include Directories Globally
zephyr_include_directories(
    modules/rowing_core/RowingData
    modules/rowing_core/RowingSettings
    modules/physics_engine/RowingEngine
    modules/physics_engine/MovingFlankDetector
    modules/physics_engine/MovingAverager
    modules/hardware_driver/GpioTimerService
    modules/hardware_driver/FakeISR
    modules/hardware_driver/InputTimerService
    modules/ble_service/BleManager
    modules/ble_service/FTMS
    modules/ble_service/RowerBridge
    modules/utilities/SystemMonitor
)
