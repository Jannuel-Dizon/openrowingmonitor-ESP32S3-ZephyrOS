# Open Rowing Monitor (ESP32-S3 / Zephyr RTOS Port)

**A real-time rowing performance monitor ported to the ESP32-S3 using Zephyr RTOS.**

This project is a complete C++ port of the excellent [Open Rowing Monitor](https://github.com/laberning/openrowingmonitor) by Lars Berning. While the original runs on a Raspberry Pi with Node.js, this version is re-engineered to run on the **ESP32-S3** microcontroller using the **Zephyr Real-Time Operating System**.

## 🎯 Project Goal
The primary goal of this project is to implement a professional-grade embedded system that replicates the complex physics engine of a rowing monitor within the constraints of a microcontroller.

It serves as a reference implementation for:
* **Zephyr RTOS** on ESP32 (Threads, Timers, DeviceTree, ISRs).
* **Real-time Physics** calculations (ported from JavaScript to C++).
* **Bluetooth LE** (FTMS Profile) and **Wi-Fi** networking.
* **Dockerized Development** workflows for embedded systems.

## 🚀 Features
* **🚣 Physics Engine:** Calculates Drive/Recovery phases, Power (Watts), Distance, and Drag Factor based on flywheel impulse timing.
* **📱 Web Dashboard:** Hosts a local web server on the ESP32 to display real-time metrics (Speed, SPM, Force Curve) on your phone.
* **🔵 Bluetooth FTMS:** Acts as a standard Fitness Machine Service, compatible with apps like **Zwift, Kinomap, and EXR**.
* **💾 Data Logging:** Records workout sessions to an SD Card in `.tcx` format.
* **☁️ Strava Integration:** Uploads completed `.tcx` files directly to Strava via Wi-Fi.

## 🛠 Hardware Requirements
* **Microcontroller:** ESP32-S3 (DevKitC-1 or similar).
* **Sensor:** Reed Switch or Optical Sensor (connected to GPIO).
* **Magnet:** Neodymium magnet attached to the rowing machine flywheel.
* **Storage:** MicroSD Card Module (SPI interface).

| Component | ESP32-S3 Pin (Default) | Notes |
| :--- | :--- | :--- |
| **Reed Switch** | `GPIO 17` | Input Pull-up |
| **SD Card CS** | `GPIO 5` | SPI Chip Select |
| **SD Card MOSI** | `GPIO 11` | |
| **SD Card MISO** | `GPIO 13` | |
| **SD Card SCK** | `GPIO 12` | |

*(Note: Pinout can be modified in `app.overlay`)*

## 📂 Software Architecture
This project uses a modular Zephyr application structure:

```text
├── app.overlay          # DeviceTree definitions (Sensor/SD Card pins)
├── prj.conf             # Zephyr Kconfig (Enables BLE, WiFi, C++, FPU)
└── src/
    ├── main.cpp         # Main thread & Initialization
    ├── engine/          # The Physics Core (Ported logic)
    │   ├── RowingEngine.cpp
    │   └── ...
    ├── ble/             # Bluetooth FTMS Service Implementation
    ├── gpio/            # High-precision Interrupt Service Routines (ISRs)
    └── web/             # Embedded Web Server & Assets
```

## 📦 Development Environment (Docker)

This project uses a containerized build system to ensure a consistent environment across different machines. You do not need to install the Zephyr SDK locally.

### Prerequisites

* [Docker Desktop](https://www.docker.com/) (or Docker Engine on Linux)
* VS Code or [Zed](https://zed.dev/) (Remote SSH capability recommended)

### Quick Start
Clone the repository:
```Bash
git clone [https://github.com/yourusername/open-rowing-monitor-esp32.git](https://github.com/yourusername/open-rowing-monitor-esp32.git)
cd open-rowing-monitor-esp32
```

Build and Run the Container:
```Bash
./build.sh  # Builds the Docker image (installs Zephyr SDK)
./run.sh    # Starts the container and drops you into a shell
```

Build the Firmware (inside the container):
```Bash
west build -b esp32s3_devkitm -p
```

Flash to Device: Since the container has access to USB (via --device=/dev/ttyUSB0 in run.sh), you can flash directly:
```Bash
west flash
```

## 🔧 Porting Notes (JS to C++)
The core physics logic was ported from the original Node.js implementation:

* Timing: Converted process.hrtime to Zephyr's k_cycle_get_32() for microsecond precision.

* Event Loop: Replaced the JS Event Loop with dedicated Zephyr threads (Sensor Priority > Physics Priority > UI Priority).

* Math: Enabled Hardware FPU (CONFIG_FPU=y) to handle the complex drag factor equations efficiently.

## 🤝 Attribution & Credits
This project is heavily based on [Open Rowing Monitor](https://github.com/laberning/openrowingmonitor) by Lars Berning.

* Original Concept & Physics Logic: Laberning

* Web Interface Design: Laberning

* Zephyr Port & ESP32 Implementation: Jannuel Lauro T. Dizon

## 📄 License
This project is licensed under the Apache 2.0 License - see the LICENSE file for details.
