cmake_minimum_required(VERSION 3.22.0)

set(ZEPHYR_EXTRA_MODULES
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rowing_core/RowingData
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/rowing_core/RowingSettings
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/physics_engine/RowingEngine
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/physics_engine/MovingFlankDetector
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/physics_engine/MovingAverager
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/hardware_driver/GpioTimerService
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(ESP32_ORM)

target_sources(app PRIVATE src/main.cpp)

# 2. Add Include Directories Globally
# You must list every folder that contains a .h file you want to #include "Filename.h"
zephyr_include_directories(
    modules/rowing_core/RowingData
    modules/rowing_core/RowingSettings
    modules/physics_engine/RowingEngine
    modules/physics_engine/MovingFlankDetector
    modules/physics_engine/MovingAverager
    modules/hardware_driver/GpioTimerService
)

# # 3. Add Sources directly to App
# # This bypasses the module linking complexity
# target_sources(app PRIVATE
#     modules/physics_engine/RowingEngine/RowingEngine.cpp
#     modules/physics_engine/MovingFlankDetector/MovingFlankDetector.cpp
#     modules/physics_engine/MovingAverager/MovingAverager.cpp
#     modules/hardware_driver/GpioTimerService/GpioTimerService.cpp
# )
