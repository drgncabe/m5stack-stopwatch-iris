# Iris

Iris is an experimental modular firmware project for the M5Stack StopWatch. It started as a watch face and settings menu, and is growing into a small app-style platform for the round StopWatch display.

Iris is not a full operating system. The current direction is simpler and more realistic for an ESP32-S3 wearable:

```text
Apps own user experiences.
Services provide reusable capabilities.
```

That means features such as the watch face, settings, developer tools, and fidgets should behave like independent user-facing apps, while shared logic such as WiFi, time, storage, battery, haptics, display power, and IMU handling should live in reusable services.

This project was created with substantial AI assistance and is being reviewed and tested on real hardware as it evolves.

## Hardware

Iris targets the M5Stack StopWatch C152:

<img width="300" height="300" alt="M5Stack StopWatch" src="https://github.com/user-attachments/assets/1f85d0ce-9610-452b-96bc-19f333b0e489" />

Relevant hardware:

- ESP32-S3
- 1.75-inch round AMOLED display
- 466x466 resolution
- Capacitive touch
- Physical A/B controls
- BMI270 6-axis IMU with accelerometer and gyroscope
- RTC
- WiFi support
- Bluetooth-capable ESP32-S3 hardware
- Speaker/audio hardware
- Vibration motor for haptics
- Battery-powered portable form factor
- M5PM1/M5IOE1 power and peripheral control hardware

The UI is designed specifically for a small circular AMOLED screen, not a generic rectangular ESP32 display.

## Current Status

Iris is early firmware. It builds, runs, and has been tested on a physical M5Stack StopWatch, but APIs, storage keys, menu structure, and hardware behavior may still change.

Current firmware version: `0.2.4`

### Implemented

- Watch face with time, date, battery level, WiFi status, theme support, and configurable widgets
- Watch registered as `WatchApp` with stable app ID `system.watch`
- Settings registered as `SettingsApp` with stable app ID `system.settings`
- Development tools registered as `DevelopmentApp` with stable app ID `system.development`
- Built-in themes with different colors and watch layouts
- First complication foundation, currently including an uptime complication
- Five-position wheel-style menu UI optimized for the circular display
- Touch scrolling with press-and-hold selection behavior and haptic feedback
- Volume, display, touch delay, theme/widget, power, WiFi, and developer settings
- Date & Time settings with country/region defaults, date format, time format, timezone, automatic NTP sync, Sync Now, manual date/time, and RTC information
- WiFi client mode with saved credentials
- WiFi setup access point for provisioning
- Lightweight web control panel while WiFi is active
- Display dimming, screen sleep, low-power watch face, and WiFi-on-demand settings
- Developer bootloader/download-mode entry from the device menu
- BMI270-based auto-rotation option
- IMU calibration screen with stored calibration data
- Fidget screens using touch, haptics, and motion data
- Status light setting

### In Development

- More deliberate app-based architecture
- Shared service layer boundaries
- Lightweight event bus for core app/service events
- More complete power-management architecture
- Improved web configurator with proper settings categories
- Better IMU/orientation calibration behavior
- Cleaner display preview/snapshot model for the web UI

### Planned

- AI/Ollama chat app
- Network scanner app
- ESP32-C5-Zero companion-node service
- GPS/location support
- Notifications
- More complications
- More fidget and motion apps
- Expanded event bus usage between services and apps
- More complete app lifecycle model

Planned features are roadmap items unless they are listed under Implemented.

## Controls

- On the watch face, tap the screen or press BtnA to open the main menu.
- In menus, drag up or down to move through items.
- Press and hold the selected item to open it.
- BtnA moves to the next item.
- BtnB selects the current item.
- Most settings screens show their available A/B actions near the bottom of the display.

## User Interface

Menus use a five-position circular-display layout:

```text
        Item -2

      Item -1

    > SELECTED <

      Item +1

        Item +2
```

The center item is largest and brightest. Nearby items are smaller and dimmer, and only up to five items are visible at once. The right-side scroll indicator appears only while scrolling or shortly after scroll activity.

Iris favors true black or very dark backgrounds where practical because the StopWatch uses an AMOLED display.

## Architecture

The current codebase is still screen-oriented, but the target architecture is app-based:

```text
                       IRIS CORE
                          |
          +---------------+---------------+
          |                               |
       APP LAYER                     SERVICE LAYER
          |                               |
   User-facing features             Shared capabilities
          |                               |
   Watch                           WiFi
   Settings                        Time / RTC
   Developer                       Storage
   Fidgets                         IMU
   Future AI Chat                  Audio / Haptics
   Future Scanner                  Power
   Future GPS                      C5 Nodes
   Future Notifications            Location
```

Current core coordination is handled mainly by `iris::App`, `AppManager`, `ServiceManager`, `EventBus`, `ScreenManager`, screen classes, and service classes. The long-term goal is to keep the Arduino entry point small:

```cpp
void setup() {
  Iris.begin();
}

void loop() {
  Iris.update();
}
```

### Applications

The application model uses stable app IDs, display names, lifecycle state, and optional lifecycle callbacks while the existing screen system is migrated incrementally. Current and candidate apps include:

- WatchApp (`system.watch`)
- SettingsApp (`system.settings`)
- DevelopmentApp (`system.development`)
- FidgetsApp
- AIChatApp
- NetworkScannerApp
- GPSApp
- NotificationsApp

The lifecycle foundation supports:

```text
Registered
Started
Paused
Stopped

begin
onStart
onPause
onResume
onStop
update
render
```

### Services

Services provide capabilities shared by multiple apps. The `ServiceManager` tracks service identity, lifecycle state, optional service callbacks, and typed access points for future app migrations. Current services include:

- `SettingsStore`
- `BatteryService`
- `OrientationService`
- `StatusLightService`
- `TimeService`
- `WifiService`
- `PowerManager`

Planned or likely future services include:

- `ImuService`
- `StorageService`
- `AudioService`
- `OllamaService`
- `C5NodeService`
- `LocationService`
- `NotificationService`

### Plugin Terminology

In Iris, "plugin" means a lightweight compile-time module/provider pattern. It does not mean DLL-style plugins, dynamically loaded ELF modules, runtime binary linking, or downloadable executable plugins.

Apps and services are compiled into the firmware, then registered, initialized, activated, suspended, or disabled as appropriate.

### Events

`EventBus` provides lightweight publish/subscribe messaging between apps and services without direct app-to-app coupling. It uses fixed subscription storage to avoid unnecessary heap churn on the ESP32-S3.

Current core events include:

- `WIFI_CONNECTED`
- `WIFI_DISCONNECTED`
- `IMU_ORIENTATION_CHANGED`
- `APP_STARTED`
- `APP_PAUSED`
- `APP_RESUMED`
- `APP_STOPPED`
- `BATTERY_LOW`

Likely future events include:

- `GPS_FIX_ACQUIRED`
- `LOCATION_CHANGED`
- `C5_NODE_DISCOVERED`
- `NETWORK_SCAN_COMPLETE`
- `NOTIFICATION_RECEIVED`
- `AI_RESPONSE_RECEIVED`

## IMU And Motion

The StopWatch includes a BMI270 6-axis IMU. Iris currently uses it for auto-rotation experiments, calibration storage, and motion-based fidgets.

The direction is to move raw BMI270 handling behind a shared IMU service:

```text
BMI270
   |
   v
ImuService
   |
   +-- calibration
   +-- calibrated acceleration
   +-- calibrated gyro
   +-- orientation
   +-- motion information
   |
   +-- Iris apps
```

Current calibration stores versioned accelerometer and gyroscope reference data for Up, Down, Left, and Right poses. Face Up and Face Down references exist in the data model for future expansion.

## WiFi

Iris can connect to a saved WiFi network or start its own setup access point.

To provision WiFi:

1. Open `Settings > WiFi`.
2. Choose `Setup`.
3. Connect a phone or computer to the `Iris-Setup-####` network.
4. Open `http://192.168.4.1`.
5. Select a network and save the password.

Credentials are stored locally on the StopWatch using ESP32 non-volatile storage and are not committed to this repository.

## Web Control Panel

When WiFi is active, Iris runs a lightweight web control panel on the device. The current web UI exposes common controls, category-based settings pages, and a text-based status snapshot.

The web configurator is moving toward a menu-based configuration console with pages for Dashboard, Display, Touch, Sound, WiFi, Theme, Power, Device, and Development. It uses the same settings model as the device UI and provides finer control than is practical on a 1.75-inch screen.

A rendered watch-face preview or framebuffer-style display snapshot is planned, but the current snapshot is text-based.

Current web/API routes include:

- `/` for the web configurator
- `/display.txt` for the plain-text device snapshot
- `/api/settings` for the combined JSON settings snapshot
- `/api/settings/display` for display-focused JSON settings
- `/api/settings/time` for Date & Time JSON settings
- `/api/settings/touch` for touch-focused JSON settings
- `/api/settings/sound` for sound-focused JSON settings
- `/api/settings/theme` for theme-focused JSON settings
- `/api/settings/power` for power-focused JSON settings
- `/api/wifi/status` for WiFi status JSON
- `/api/wifi/networks` for nearby WiFi network scan results

## ESP32-C5-Zero Companion Nodes

ESP32-C5-Zero integration is planned. The intended model is a shared C5 node service, not scanner-specific code baked into one app.

Conceptually:

```text
NetworkScannerApp
        |
        v
   C5NodeService
        |
   +----+----+
   |         |
 C5 Node   C5 Node
```

The service may eventually provide node discovery, node status, capability discovery, command dispatch, telemetry, WiFi scanning, BLE scanning, and future radio/sensor features. None of that should be treated as complete until implemented.

## Development

Primary development environment:

```text
Visual Studio Code
PlatformIO
Arduino / ESP32
```

Current PlatformIO configuration:

```ini
[env:m5stack-stopwatch]
platform = espressif32 @ 6.12.0
board = esp32s3box
framework = arduino
```

Key libraries:

- M5Unified
- M5GFX
- M5PM1
- M5IOE1
- WiFi
- Preferences
- WebServer

Build:

```bash
pio run
```

Upload:

```bash
pio run -t upload
```

Serial monitor:

```bash
pio device monitor
```

Compile validation means the firmware builds successfully. It does not prove every physical hardware behavior has been validated.

## Project Structure

Current structure:

```text
include/iris/
  App.h
  AppConfig.h
  Theme.h
  apps/
  screens/
  services/

src/
  App.cpp
  apps/
  main.cpp
  screens/
  services/
```

Target direction:

```text
src/
  core/
  apps/
  services/
  drivers/
  main.cpp
```

The target structure is not fully implemented yet.

## Adding Features

For current code, most user-facing additions start as a new `Screen` plus a menu entry. Shared behavior should go into a service instead of being duplicated inside individual screens.

Near-term direction:

- New user-facing feature: add an app/screen and register it centrally.
- Shared hardware or protocol behavior: add or extend a service.
- Cross-feature communication: use the planned event system once it exists.

## Roadmap

- Refactor toward a true app manager and service manager
- Add a centralized power manager with app/service power requirements
- Redesign the web configurator around settings categories
- Expand IMU calibration and orientation handling
- Add more complications and watch layouts
- Add AI/Ollama chat
- Add network scanning and C5 companion-node support
- Add GPS/location features
- Add notification handling

## License

Iris is released under the MIT License. See [LICENSE](LICENSE) for details.
