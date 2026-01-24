# Open Rowing Monitor - ESP32 Production Guide

## Version 1.0.0

This guide covers building and deploying the production-ready firmware for the Open Rowing Monitor on ESP32-S3.

---

## Quick Start

### 1. Build Production Firmware

```bash
cd workspace/open_rowing_monitor_esp32
west build -b esp32s3_devkitc/esp32s3/procpu
```

### 2. Flash to Device

```bash
west flash
```

### 3. Monitor Output

```bash
west espressif monitor
```

---

## Configuration Profiles

### Production Build (Default)
- File: `prj.conf`
- Optimized for size and performance
- Debug features disabled
- Log level: INFO
- Use this for end-users

### Debug Build
- File: `prj_debug.conf`
- All monitoring enabled
- Full debug symbols
- Log level: DEBUG
- Use this for development

To build with debug config:
```bash
west build -b esp32s3_devkitc/esp32s3/procpu -- -DCONF_FILE=prj_debug.conf
```

---

## Hardware Requirements

- **MCU**: ESP32-S3 (any variant with ≥8MB flash)
- **Magnet Sensor**: Reed switch or Hall effect on GPIO 17
- **Power Button** (optional): GPIO 13
- **Power**: USB-C or 5V supply

### Pin Configuration

Edit `boards/esp32s3_devkitc_esp32s3_procpu.overlay` to change pins:

```dts
rower_reed_switch: reed_switch {
    gpios = <&gpio0 17 (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
};
```

---

## Calibration

### Finding Your Flywheel Inertia

1. Build with debug config
2. Enable profiling: `CONFIG_GPIO_ENABLE_PHYSICS_PROFILING=y`
3. Row at constant pace
4. Adjust `CONFIG_ORM_FLYWHEEL_INERTIA_X10000` until power readings match known values

### Finding Your Magic Constant

The magic constant converts angular velocity to linear velocity (m/s).

**Default: 2.8** (Concept2 standard)

If your distance readings are off:
- Too many meters → Increase `CONFIG_ORM_MAGIC_CONSTANT_X10000`
- Too few meters → Decrease it

### Finding Your Drag Factor

**Auto-adjust is enabled by default** (`CONFIG_ORM_AUTO_ADJUST_DRAG_FACTOR=y`)

If you prefer manual tuning:
1. Set `CONFIG_ORM_AUTO_ADJUST_DRAG_FACTOR=n`
2. Row at known power output (e.g., 100W on another device)
3. Adjust `CONFIG_ORM_DRAG_FACTOR` until it matches

---

## Building for Different Rowers

### Example: Concept2 Model D

```kconfig
CONFIG_ORM_IMPULSES_PER_REV=1
CONFIG_ORM_FLYWHEEL_INERTIA_X10000=1000
CONFIG_ORM_MAGIC_CONSTANT_X10000=2800
CONFIG_ORM_DRAG_FACTOR=1500
```

### Example: WaterRower (WRX700)

```kconfig
CONFIG_ORM_IMPULSES_PER_REV=6
CONFIG_ORM_FLYWHEEL_INERTIA_X10000=7200
CONFIG_ORM_MAGIC_CONSTANT_X10000=2800
CONFIG_ORM_DRAG_FACTOR=3000
```

### Example: Magnetic Rower (Generic)

```kconfig
CONFIG_ORM_IMPULSES_PER_REV=3
CONFIG_ORM_FLYWHEEL_INERTIA_X10000=5000
CONFIG_ORM_MAGIC_CONSTANT_X10000=2800
CONFIG_ORM_DRAG_FACTOR=2500
```

---

## Connecting to Apps

The device advertises as **"Rowing-Monitor"** and implements the FTMS (Fitness Machine Service) Bluetooth profile.

### Compatible Apps
- **Kinomap** (iOS/Android)
- **EXR** (iOS/Android)
- **Zwift** (via Concept2 PM5 emulation)
- **Any app supporting FTMS Rower Data**

### Connection Steps
1. Power on the ESP32
2. Open your rowing app
3. Scan for Bluetooth devices
4. Select "Rowing-Monitor"
5. Start rowing!

---

## Troubleshooting

### Device Not Advertising

**Check:**
- BLE is enabled: `CONFIG_BT=y`
- Device name is set: `CONFIG_BT_DEVICE_NAME="Rowing-Monitor"`
- Monitor logs for errors: `west espressif monitor`

### Erratic Power Readings

**Try:**
1. Increase smoothing: `CONFIG_ORM_SMOOTHING=5`
2. Increase flank length: `CONFIG_ORM_FLANK_LENGTH=4`
3. Check magnet sensor wiring (pull-up resistor needed)

### App Disconnects Frequently

**Check:**
- BLE buffer sizes in `prj.conf`
- Connection interval settings
- Phone Bluetooth power saving mode

### Stack Overflow Errors

Build with debug config to see which thread:
```bash
west build -b esp32s3_devkitc/esp32s3/procpu -- -DCONF_FILE=prj_debug.conf
```

Then increase the relevant stack size in `prj.conf`.

---

## Performance Metrics

### Current Production Build

- **Flash Used**: ~650KB / 16MB
- **RAM Used**: ~180KB / 512KB
- **Update Rate**: 4Hz (250ms intervals)
- **BLE Latency**: <50ms typical
- **Power Draw**: ~120mA @ 3.3V (active rowing)

---

## Release Checklist

Before releasing a new version:

- [ ] Update version in `version.h`
- [ ] Test with production config (`prj.conf`)
- [ ] Verify all apps connect successfully
- [ ] Check power readings against known device
- [ ] Test session start/stop
- [ ] Test multi-connection (if enabled)
- [ ] Monitor for memory leaks (24hr test)
- [ ] Update CHANGELOG.md
- [ ] Tag release in git

---

## License

Based on [Open Rowing Monitor](https://github.com/laberning/openrowingmonitor) by Jaap van Ekris and Lars Berning.

ESP32 port by [Jannuel Dizon] - Licensed under GPL-3.0

---

## Support

- **Issues**: GitHub Issues
- **Discussions**: GitHub Discussions
- **Email**: [jannuellaurodizon@gmail.com]
