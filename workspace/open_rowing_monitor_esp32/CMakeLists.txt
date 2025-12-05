cmake_minimum_required(VERSION 3.22.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(ESP32_ORM)

target_sources(app PRIVATE
    src/main.c

    src/engine/RowingEngine.cpp
    src/engine/MovingFlankDetector.cpp
    src/engine/WorkoutRecorder.cpp
    src/engine/averager/MovingAverager.cpp

    src/gpio/GpioTimerService.cpp

    src/ble/BleManager.cpp
    src/ble/ftms/FitnessMachineService.cpp
    src/ble/ftms/RowerDataCharacteristic.cpp

    src/web/WebServer.cpp
)

target_include_directories(app PRIVATE src)
