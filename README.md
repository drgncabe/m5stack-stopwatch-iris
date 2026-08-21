# Iris

Iris is an experimental smartwatch-style firmware for the M5Stack StopWatch. It provides a clock face, themed menus, WiFi setup, battery status, power-saving controls, a small web control panel, and developer tools for entering bootloader/download mode without relying on the hardware button sequence.

Note: This project was created for specific needs I have, for expediency this was written with AI. It originally started with me coding but I quickly slowed things down. I am reviewing code along with doing hands-on testing on real hardware, but I mainly built this for rapid prototyping and making my stopwatch a useful multi-tool. Sorry for the AI slop, but sometimes it's useful AI slop.

## Hardware

<img width="300" height="300" alt="image" src="https://github.com/user-attachments/assets/1f85d0ce-9610-452b-96bc-19f333b0e489" />

- M5Stack StopWatch C152 (https://docs.m5stack.com/en/core/StopWatch)
- ESP32-S3
- Round AMOLED touch display
- M5Stack PMIC, battery, buttons, speaker, RTC, and WiFi support

## Current Features

- Clock view with time, date, battery status, named themes, and configurable widgets
- Modular screen/menu system for adding future apps
- Settings for volume, WiFi, theme/widgets, power behavior, and touch delay
- WiFi client mode with saved credentials
- WiFi setup access point for provisioning
- Web control panel while WiFi is active
- Display dim and sleep timeouts for battery savings
- Low-power clock face option
- Developer menu with bootloader/download-mode entry
- Hardware button controls preserved from the M5Stack examples

## Controls

- On the clock face, tap the screen or press BtnA to open the main menu.
- In menus, BtnA moves to the next item and BtnB selects it.
- Touching a menu row highlights it briefly before selecting it.
- On most settings screens, BtnA and BtnB perform the primary actions shown at the bottom of the display.

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

When WiFi is active, Iris runs a lightweight web control panel on the device. The panel exposes common display actions, volume controls, WiFi setup, theme cycling, and a text snapshot of the current device state.

The display snapshot is currently text-based. A rendered image/framebuffer snapshot is planned for a later phase.

## Themes And Widgets

`Settings > Theme & widgets` lets you cycle through built-in themes and choose which clock face widgets are visible:

- Battery
- Date
- Seconds
- WiFi status

Theme selection currently reuses the original background storage key so existing test devices keep their saved visual style.

## Developer Menu

`Settings > Developer > Bootloader` enters USB download mode from the display. On the M5Stack StopWatch this uses the PMIC download-mode command first, which mirrors the hardware boot gesture more closely than a plain software restart.

## Building

This project is intended for VS Code with PlatformIO.

```ini
[env:m5stack-stopwatch]
platform = espressif32 @ 6.12.0
board = esp32s3box
framework = arduino
```

Install PlatformIO, open the project folder, then build:

```bash
pio run
```

Upload to the StopWatch:

```bash
pio run -t upload
```

Monitor serial output:

```bash
pio device monitor
```

## Project Status

Iris is early firmware. It is usable for testing, but APIs, menus, themes, and storage keys may change as the project evolves.

## AI Disclosure

This firmware and README were created with substantial AI assistance, with device behavior validated manually on real M5Stack StopWatch hardware.
