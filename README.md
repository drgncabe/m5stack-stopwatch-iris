# Iris

Iris is a modular firmware project for the M5Stack StopWatch. It is developed in Visual Studio Code with PlatformIO and is intended to grow into a multi-function wearable interface over multiple phases.

## Phase One

Phase One establishes the application architecture and the core watch/settings experience.

### Included

- Standard watch view with time, seconds, date, and weekday
- RTC-backed time display
- NTP synchronization when WiFi is available
- Modular screen manager for future features
- Main menu and Settings menu
- Persistent speaker volume setting
- WiFi enable/disable control
- WiFi provisioning without storing credentials in GitHub
- Device information screen
- Touch navigation
- M5Stack StopWatch BtnA / BtnB example behavior retained in the main loop, including the original serial messages

## Controls

### Watch

- Touch screen: open the main menu
- BtnA: open the main menu
- BtnB: play a short hardware-test tone; the press is also logged to Serial

### Menus

- Touch a row: open it
- BtnA: move to the next menu item
- BtnB: select the highlighted item

### Volume

- Touch `-` / `+`: adjust volume
- BtnA: volume down
- BtnB: volume up
- Back button: return to Settings

### WiFi

- Touch Enable/Disable: toggle WiFi
- Touch Setup: start the provisioning access point
- BtnA: toggle WiFi
- BtnB: start provisioning
- Back button: return to Settings

### Device information

- Shows Iris version, ESP32 model, CPU speed, flash, PSRAM, RTC status, WiFi status, and MAC address
- Touch Back, BtnA, or BtnB to return to Settings

## WiFi provisioning

Iris does not require WiFi credentials in source code.

1. Open **Settings > WiFi**.
2. Select **Setup** or press BtnB.
3. Iris creates an access point named `Iris-Setup-xxxx`.
4. Connect a phone or computer to that network.
5. Open `http://192.168.4.1`.
6. Select the desired WiFi network, enter its password, and save.
7. Iris stores the credentials in ESP32 NVS and reconnects as a station.

The Settings menu can later disable WiFi without deleting the saved credentials.

## Timezone

Phase One defaults to US Eastern time using the POSIX timezone string:

```text
EST5EDT,M3.2.0,M11.1.0
```

Change `kTimezone` in `include/iris/AppConfig.h` for another deployment timezone.

## Development environment

- Visual Studio Code
- PlatformIO IDE extension
- Arduino framework
- M5Stack StopWatch / ESP32-S3

The PlatformIO configuration follows M5Stack's current StopWatch guidance:

```ini
[env:m5stack-stopwatch]
platform = espressif32 @ 6.12.0
board = esp32s3box
framework = arduino
board_build.partitions = default_16MB.csv
board_upload.flash_size = 16MB
board_upload.maximum_size = 16777216
board_build.arduino.memory_type = qio_opi
```

Dependencies are pulled directly from the official M5Stack repositories for M5Unified, M5GFX, M5PM1, and M5IOE1.

## Build and upload

Clone the repository and open the folder in VS Code. PlatformIO should recognize `platformio.ini` automatically.

From the PlatformIO toolbar use **Build**, **Upload**, and **Serial Monitor**, or from a PlatformIO shell:

```bash
pio run
pio run -t upload
pio device monitor
```

If the StopWatch is not detected for upload, put it into download mode according to the M5Stack StopWatch instructions before retrying.

## Architecture

```text
include/iris/
├── App.h
├── AppConfig.h
├── screens/
│   ├── Screen.h
│   ├── ScreenManager.h
│   ├── MenuScreen.h
│   ├── WatchScreen.h
│   ├── VolumeScreen.h
│   ├── WifiScreen.h
│   └── DeviceInfoScreen.h
└── services/
    ├── SettingsStore.h
    ├── TimeService.h
    └── WifiService.h

src/
├── main.cpp
├── App.cpp
├── screens/
└── services/
```

`Screen` is the extension point for future Iris features. A later feature can be implemented as a new screen class, registered with `ScreenManager`, and exposed through a `MenuItem` without changing the application's basic navigation model.

## Phase One acceptance criteria

- Firmware boots to a watch face
- Watch displays RTC time when valid
- Connected WiFi can synchronize time using NTP
- Main menu opens by touch or BtnA
- Settings provides Volume, WiFi, and Device Information
- Volume changes are persisted between reboots
- WiFi state and credentials are persisted between reboots
- WiFi credentials are not committed to the repository
- BtnA and BtnB continue to produce the M5Stack example serial messages
- New screens can be added without rewriting the main loop

## Planned next steps

Later phases can add additional Iris modules while retaining the same screen/service architecture.
