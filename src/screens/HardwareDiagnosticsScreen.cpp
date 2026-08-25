#include "iris/screens/HardwareDiagnosticsScreen.h"

#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>

#include "iris/AppConfig.h"
#include "iris/Theme.h"
#include "iris/screens/ScreenManager.h"

namespace iris {

namespace {
constexpr int kRowHeight = 34;
constexpr int kRowStartY = 76;
constexpr int kRowLeft = 38;
constexpr int kRowWidth = 390;
constexpr int kRowRectHeight = 29;
constexpr int kRowValueLeft = 222;
constexpr int kRowValueRightPadding = 16;
constexpr int kRowValueWidth = kRowLeft + kRowWidth - kRowValueLeft - kRowValueRightPadding;
constexpr size_t kItemCount = 8;
constexpr size_t kPageCount = 12;
constexpr uint32_t kShortTestMs = 5000;
constexpr uint32_t kHapticTestMs = 650;
constexpr uint32_t kStatusRefreshMs = 500;

bool timeExpired(uint32_t nowMs, uint32_t untilMs) {
  return untilMs != 0 && static_cast<int32_t>(nowMs - untilMs) >= 0;
}

String formatBytes(uint32_t bytes) {
  if (bytes >= 1024 * 1024) {
    return String(bytes / (1024 * 1024)) + "." + String((bytes % (1024 * 1024)) / 104858) + " MB";
  }
  if (bytes >= 1024) return String(bytes / 1024) + " KB";
  return String(bytes) + " B";
}

String formatDateTime(const DateTimeSnapshot& dt) {
  if (!dt.valid) return "Not set";
  char buffer[24];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
           dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
  return String(buffer);
}

String formatVec3(float x, float y, float z, uint8_t decimals) {
  const unsigned int places = decimals;
  return String(x, places) + "," + String(y, places) + "," + String(z, places);
}

String fitTextToWidth(const String& text, int maxWidth) {
  if (maxWidth <= 0 || M5.Display.textWidth(text) <= maxWidth) return text;

  String fitted(text);
  const String ellipsis("...");
  while (fitted.length() > 0 &&
         M5.Display.textWidth(fitted + ellipsis) > maxWidth) {
    fitted.remove(fitted.length() - 1);
  }

  if (fitted.length() == 0) return ellipsis;
  return fitted + ellipsis;
}
}  // namespace

void HardwareDiagnosticsScreen::enter() {
  page_ = Page::System;
  selected_ = 0;
  lastTouchX_ = -1;
  lastTouchY_ = -1;
  audioSilenceUntilMs_ = 0;
  displayOffUntilMs_ = 0;
  brightnessTestUntilMs_ = 0;
  hapticUntilMs_ = 0;
  lastStatusDrawMs_ = 0;
  brightnessTestLabel_ = nullptr;
}

void HardwareDiagnosticsScreen::update(uint32_t nowMs) {
  bool shouldRedraw = false;

  if (timeExpired(nowMs, audioSilenceUntilMs_)) {
    restoreAudio();
    shouldRedraw = true;
  }
  if (timeExpired(nowMs, displayOffUntilMs_)) {
    restoreDisplay();
    shouldRedraw = true;
  }
  if (timeExpired(nowMs, brightnessTestUntilMs_)) {
    restoreDisplay();
    shouldRedraw = true;
  }
  if (timeExpired(nowMs, hapticUntilMs_)) {
    stopHaptic();
    shouldRedraw = true;
  }

  if (displayOffUntilMs_ != 0) return;

  if (page_ == Page::Imu && M5.Imu.isEnabled()) {
    M5.Imu.update();
  }

  if (shouldRedraw) {
    draw();
    lastStatusDrawMs_ = nowMs;
  } else if (nowMs - lastStatusDrawMs_ >= kStatusRefreshMs) {
    drawLiveValues();
    drawFooter();
    lastStatusDrawMs_ = nowMs;
  }
}

void HardwareDiagnosticsScreen::draw() {
  if (displayOffUntilMs_ != 0) return;

  const Theme theme = currentTheme(settings_);
  M5.Display.fillScreen(theme.background);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.drawString("HW diagnostics", M5.Display.width() / 2, 49);

  for (size_t i = 0; i < kItemCount; ++i) {
    drawRow(i, i == selected_);
  }

  drawFooter();
}

void HardwareDiagnosticsScreen::previewTouch(int32_t x, int32_t y) {
  lastTouchX_ = x;
  lastTouchY_ = y;
  const int row = rowAt(x, y);
  if (row < 0) return;
  selectRow(static_cast<size_t>(row));
}

void HardwareDiagnosticsScreen::handleTouch(int32_t x, int32_t y) {
  lastTouchX_ = x;
  lastTouchY_ = y;
  const int row = rowAt(x, y);
  if (row < 0) return;
  selectRow(static_cast<size_t>(row));
  activateSelected(millis());
}

void HardwareDiagnosticsScreen::onButtonA() {
  selectRow((selected_ + 1) % kItemCount);
}

void HardwareDiagnosticsScreen::onButtonB() {
  activateSelected(millis());
}

void HardwareDiagnosticsScreen::activateSelected(uint32_t nowMs) {
  if (selected_ == 0) {
    nextPage();
    return;
  }
  if (selected_ == kItemCount - 1) {
    goBack();
    return;
  }

  switch (page_) {
    case Page::Display:
      if (selected_ == 3) startDisplayOff(nowMs);
      if (selected_ == 4) startBrightnessTest(16, "Min test", nowMs);
      if (selected_ == 5) startBrightnessTest(200, "High test", nowMs);
      break;
    case Page::Audio:
      if (selected_ == 2) testAudioTone();
      if (selected_ == 3) startAudioSilence(nowMs);
      break;
    case Page::Wifi:
      if (selected_ == 4) toggleWifi();
      if (selected_ == 5) toggleWifiSleep();
      break;
    case Page::Power:
      if (selected_ == 4) cyclePowerProfile();
      break;
    case Page::Haptics:
      if (selected_ == 1) startHapticPulse(nowMs);
      break;
    case Page::StatusLight:
      if (selected_ == 4) testStatusLightColor(0, 48, 0);
      if (selected_ == 5) testStatusLightColor(48, 0, 0);
      if (selected_ == 6) clearStatusLight();
      break;
    default:
      break;
  }

  if (displayOffUntilMs_ == 0) {
    drawRow(selected_, true);
    drawFooter();
  }
}

void HardwareDiagnosticsScreen::selectRow(size_t index) {
  if (index >= kItemCount || index == selected_ || displayOffUntilMs_ != 0) return;
  const size_t previous = selected_;
  selected_ = index;
  drawRow(previous, false);
  drawRow(selected_, true);
}

void HardwareDiagnosticsScreen::nextPage() {
  const uint8_t next = (static_cast<uint8_t>(page_) + 1) % kPageCount;
  page_ = static_cast<Page>(next);
  selected_ = 0;
  draw();
}

void HardwareDiagnosticsScreen::drawRow(size_t index, bool selected) {
  if (index >= kItemCount || displayOffUntilMs_ != 0) return;

  const Theme theme = currentTheme(settings_);
  const int y = kRowStartY + static_cast<int>(index) * kRowHeight;
  const uint16_t fill = selected ? theme.selected : theme.background;
  const uint16_t border = selected ? theme.foreground : theme.panel;

  const char* label = rowLabel(index);
  const String value = rowValue(index);

  M5.Display.fillRoundRect(kRowLeft, y, kRowWidth, kRowRectHeight, 10, fill);
  M5.Display.drawRoundRect(kRowLeft, y, kRowWidth, kRowRectHeight, 10, border);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextDatum(middle_left);
  M5.Display.setTextColor(theme.foreground, fill);
  M5.Display.drawString(label, kRowLeft + 16, y + (kRowRectHeight / 2));
  M5.Display.setTextDatum(middle_right);
  M5.Display.setTextColor(theme.muted, fill);
  M5.Display.drawString(fitTextToWidth(value, kRowValueWidth), kRowLeft + kRowWidth - 16,
                        y + (kRowRectHeight / 2));
}

void HardwareDiagnosticsScreen::drawLiveValues() {
  if (displayOffUntilMs_ != 0) return;

  for (size_t i = 1; i < kItemCount - 1; ++i) {
    drawRowValue(i);
  }
}

void HardwareDiagnosticsScreen::drawRowValue(size_t index) {
  if (index >= kItemCount || displayOffUntilMs_ != 0) return;

  const Theme theme = currentTheme(settings_);
  const int y = kRowStartY + static_cast<int>(index) * kRowHeight;
  const bool selected = index == selected_;
  const uint16_t fill = selected ? theme.selected : theme.background;

  M5.Display.fillRect(kRowValueLeft, y + 2,
                      kRowLeft + kRowWidth - kRowValueLeft - 2,
                      kRowRectHeight - 4, fill);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextDatum(middle_right);
  M5.Display.setTextColor(theme.muted, fill);
  M5.Display.drawString(fitTextToWidth(rowValue(index), kRowValueWidth),
                        kRowLeft + kRowWidth - kRowValueRightPadding,
                        y + (kRowRectHeight / 2));
}

void HardwareDiagnosticsScreen::drawFooter() {
  if (displayOffUntilMs_ != 0) return;

  const Theme theme = currentTheme(settings_);
  M5.Display.fillRect(18, 418, 430, 36, theme.background);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(theme.muted, theme.background);
  String status = "CPU ";
  status += String(power_.currentCpuMhz());
  status += " MHz  ";
  status += battery_.statusText();
  M5.Display.drawString(status, M5.Display.width() / 2, 428);
  M5.Display.drawString("A: Next     B: Run/Page", M5.Display.width() / 2, 448);
}

int HardwareDiagnosticsScreen::rowAt(int32_t x, int32_t y) const {
  if (x < kRowLeft || x > kRowLeft + kRowWidth) return -1;
  for (size_t i = 0; i < kItemCount; ++i) {
    const int rowY = kRowStartY + static_cast<int>(i) * kRowHeight;
    if (y >= rowY - 3 && y <= rowY + kRowRectHeight + 3) return static_cast<int>(i);
  }
  return -1;
}

const char* HardwareDiagnosticsScreen::pageName() const {
  switch (page_) {
    case Page::System: return "System";
    case Page::Memory: return "Memory";
    case Page::Display: return "Display";
    case Page::Audio: return "Audio";
    case Page::Input: return "Input";
    case Page::Imu: return "BMI270";
    case Page::Wifi: return "WiFi";
    case Page::Bluetooth: return "BLE";
    case Page::Power: return "Power";
    case Page::Rtc: return "RTC";
    case Page::Haptics: return "Haptics";
    case Page::StatusLight: return "Status light";
  }
  return "Unknown";
}

const char* HardwareDiagnosticsScreen::rowLabel(size_t index) const {
  if (index == 0) return "Page";
  if (index == kItemCount - 1) return "Back";

  switch (page_) {
    case Page::System: {
      constexpr const char* labels[] = {"", "Firmware", "CPU", "Uptime", "Heap", "PSRAM", "Flash", ""};
      return labels[index];
    }
    case Page::Memory: {
      constexpr const char* labels[] = {"", "Heap", "Min heap", "PSRAM free", "PSRAM total", "Sketch used", "Sketch free", ""};
      return labels[index];
    }
    case Page::Display: {
      constexpr const char* labels[] = {"", "Brightness", "Power state", "Display off", "Brightness min", "Brightness high", "Rotation", ""};
      return labels[index];
    }
    case Page::Audio: {
      constexpr const char* labels[] = {"", "Volume", "Tone test", "Silence test", "Speaker", "Mic", "Restore", ""};
      return labels[index];
    }
    case Page::Input: {
      constexpr const char* labels[] = {"", "Touch", "BtnA", "BtnB", "Delay", "Preview", "Coordinates", ""};
      return labels[index];
    }
    case Page::Imu: {
      constexpr const char* labels[] = {"", "Sensor", "Accel", "Gyro", "Auto rotate", "Rotation", "Offsets", ""};
      return labels[index];
    }
    case Page::Wifi: {
      constexpr const char* labels[] = {"", "Status", "SSID", "IP", "Radio", "Sleep", "Portal", ""};
      return labels[index];
    }
    case Page::Bluetooth: {
      constexpr const char* labels[] = {"", "Status", "Device", "Profile", "Advertise", "Connected", "Bonded", ""};
      return labels[index];
    }
    case Page::Power: {
      constexpr const char* labels[] = {"", "Battery", "Charging", "Display", "Profile", "CPU", "Idle dim/off", ""};
      return labels[index];
    }
    case Page::Rtc: {
      constexpr const char* labels[] = {"", "RTC", "Clock", "NTP", "Timezone", "System time", "Sync", ""};
      return labels[index];
    }
    case Page::Haptics: {
      constexpr const char* labels[] = {"", "Pulse test", "Vibration", "Audio mute", "Display test", "WiFi sleep", "Safe restore", ""};
      return labels[index];
    }
    case Page::StatusLight: {
      constexpr const char* labels[] = {"", "Driver", "PM1 G0", "Boot use", "Test green", "Test red", "Test off", ""};
      return labels[index];
    }
  }
  return "";
}

String HardwareDiagnosticsScreen::rowValue(size_t index) const {
  if (index == 0) return String(pageName()) + " >";
  if (index == kItemCount - 1) return "";

  switch (page_) {
    case Page::System:
      switch (index) {
        case 1: return config::kVersion;
        case 2: return String(getCpuFrequencyMhz()) + " MHz";
        case 3: return String(millis() / 1000) + "s";
        case 4: return formatBytes(ESP.getFreeHeap());
        case 5: return psramFound() ? formatBytes(ESP.getFreePsram()) : "Unavailable";
        case 6: return formatBytes(ESP.getFlashChipSize());
      }
      break;
    case Page::Memory:
      switch (index) {
        case 1: return formatBytes(ESP.getFreeHeap());
        case 2: return formatBytes(ESP.getMinFreeHeap());
        case 3: return psramFound() ? formatBytes(ESP.getFreePsram()) : "Unavailable";
        case 4: return psramFound() ? formatBytes(ESP.getPsramSize()) : "Unavailable";
        case 5: return formatBytes(ESP.getSketchSize());
        case 6: return formatBytes(ESP.getFreeSketchSpace());
      }
      break;
    case Page::Display:
      switch (index) {
        case 1: return String(settings_.activeBrightness()) + "/255";
        case 2: return power_.stateName();
        case 3: return displayOffUntilMs_ == 0 ? "5 sec" : "Off";
        case 4: return brightnessTestLabel_ == nullptr ? "5 sec" : brightnessTestLabel_;
        case 5: return brightnessTestLabel_ == nullptr ? "5 sec" : brightnessTestLabel_;
        case 6: return String(M5.Display.getRotation());
      }
      break;
    case Page::Audio:
      switch (index) {
        case 1: return String((settings_.volume() * 100) / 255) + "%";
        case 2: return "Run";
        case 3: return audioSilenceUntilMs_ == 0 ? "5 sec" : "Muted";
        case 4: return "Available";
        case 5: return "Not measured";
        case 6: return "On exit";
      }
      break;
    case Page::Input:
      switch (index) {
        case 1: return M5.Touch.getDetail().isPressed() ? "Pressed" : "Released";
        case 2: return M5.BtnA.isPressed() ? "Pressed" : "Released";
        case 3: return M5.BtnB.isPressed() ? "Pressed" : "Released";
        case 4: return String(settings_.touchDelayMs()) + "ms";
        case 5: return lastTouchX_ < 0 ? "Waiting" : "Seen";
        case 6:
          return lastTouchX_ < 0 ? "None" : String(lastTouchX_) + "," + String(lastTouchY_);
      }
      break;
    case Page::Imu:
      if (!M5.Imu.isEnabled()) return index == 1 ? "Unavailable" : "--";
      {
        float ax = 0.0f;
        float ay = 0.0f;
        float az = 0.0f;
        float gx = 0.0f;
        float gy = 0.0f;
        float gz = 0.0f;
        M5.Imu.getAccel(&ax, &ay, &az);
        M5.Imu.getGyro(&gx, &gy, &gz);
        switch (index) {
          case 1: return "Enabled";
          case 2: return formatVec3(ax, ay, az, 2);
          case 3: return formatVec3(gx, gy, gz, 0);
          case 4: return settings_.autoRotate() ? "On" : "Off";
          case 5: return orientation_.statusText();
          case 6:
            return formatVec3(settings_.accelOffsetX(), settings_.accelOffsetY(),
                              settings_.accelOffsetZ(), 2);
        }
      }
      break;
    case Page::Wifi:
      switch (index) {
        case 1: return wifi_.statusText();
        case 2: return wifi_.ssid().length() == 0 ? "None" : wifi_.ssid();
        case 3: return wifi_.ipAddress();
        case 4: return wifi_.isEnabled() ? "On" : "Off";
        case 5: return wifiSleep_ ? "On" : "Off";
        case 6: return wifi_.isProvisioning() ? wifi_.portalSsid() : "Stopped";
      }
      break;
    case Page::Bluetooth:
      switch (index) {
        case 1: return bluetooth_.statusText();
        case 2: return bluetooth_.deviceName();
        case 3: return "BLE HID";
        case 4: return bluetooth_.advertising() ? "Yes" : "No";
        case 5: return bluetooth_.connected() ? "Yes" : "No";
        case 6: return String(bluetooth_.bondedDeviceCount());
      }
      break;
    case Page::Power:
      switch (index) {
        case 1: return battery_.statusText();
        case 2: {
          const auto charging = M5.Power.isCharging();
          if (charging == m5::Power_Class::charge_unknown) return "Unknown";
          return charging == m5::Power_Class::is_charging ? "Charging" : "No";
        }
        case 3: return power_.stateName();
        case 4: return power_.profileName();
        case 5: return String(power_.currentCpuMhz()) + " MHz";
        case 6:
          return String(settings_.dimTimeoutSeconds()) + "/" +
                 String(settings_.sleepTimeoutSeconds()) + "s";
      }
      break;
    case Page::Rtc:
      switch (index) {
        case 1: return timeService_.rtcAvailable() ? "Available" : "Unavailable";
        case 2: return formatDateTime(timeService_.now());
        case 3: return timeService_.ntpSynchronized() ? "Synced" : "Pending";
        case 4: return timeZoneIanaName(settings_.timeZone());
        case 5: return timeService_.now().valid ? "Valid" : "Invalid";
        case 6: return wifi_.isConnected() ? "WiFi ready" : "Needs WiFi";
      }
      break;
    case Page::Haptics:
      switch (index) {
        case 1: return hapticUntilMs_ == 0 ? "Run" : "On";
        case 2: return "M5PM1";
        case 3: return audioSilenceUntilMs_ == 0 ? "Restored" : "Muted";
        case 4: return displayOffUntilMs_ == 0 ? "Restored" : "Off";
        case 5: return wifiSleep_ ? "On" : "Off";
        case 6: return "On back";
      }
      break;
    case Page::StatusLight:
      switch (index) {
        case 1: return statusLight_.capabilityText();
        case 2: return "Wake/Neo";
        case 3: return "Forced off";
        case 4: return statusLight_.available() ? "Run" : "No effect";
        case 5: return statusLight_.available() ? "Run" : "No effect";
        case 6: return statusLight_.available() ? "Run" : "No effect";
      }
      break;
  }

  return "";
}

void HardwareDiagnosticsScreen::startAudioSilence(uint32_t nowMs) {
  M5.Speaker.setVolume(0);
  audioSilenceUntilMs_ = nowMs + kShortTestMs;
}

void HardwareDiagnosticsScreen::testAudioTone() {
  M5.Speaker.setVolume(settings_.volume());
  if (settings_.volume() > 0) M5.Speaker.tone(2400, 90);
}

void HardwareDiagnosticsScreen::startDisplayOff(uint32_t nowMs) {
  restoreAudio();
  stopHaptic();
  displayOffUntilMs_ = nowMs + kShortTestMs;
  M5.Display.sleep();
}

void HardwareDiagnosticsScreen::startBrightnessTest(uint8_t brightness, const char* label, uint32_t nowMs) {
  restoreDisplay();
  brightnessTestLabel_ = label;
  brightnessTestUntilMs_ = nowMs + kShortTestMs;
  M5.Display.setBrightness(brightness);
}

void HardwareDiagnosticsScreen::toggleWifi() {
  const bool enabled = !wifi_.isEnabled();
  settings_.setWifiEnabled(enabled);
  wifi_.setEnabled(enabled);
}

void HardwareDiagnosticsScreen::toggleWifiSleep() {
  wifiSleep_ = !wifiSleep_;
  if (wifi_.isEnabled()) WiFi.setSleep(wifiSleep_);
}

void HardwareDiagnosticsScreen::startHapticPulse(uint32_t nowMs) {
  M5.Power.setVibration(90);
  hapticUntilMs_ = nowMs + kHapticTestMs;
}

void HardwareDiagnosticsScreen::testStatusLightColor(uint8_t red, uint8_t green, uint8_t blue) {
  Serial.printf("[HWDiag] Status light diagnostic RGB(%u,%u,%u); capability=%s\n",
                red, green, blue, statusLight_.capabilityText());
  statusLight_.showNotification(red, green, blue);
  statusLight_.setEnabled(true);
}

void HardwareDiagnosticsScreen::clearStatusLight() {
  Serial.printf("[HWDiag] Status light diagnostic off; capability=%s\n",
                statusLight_.capabilityText());
  statusLight_.clearNotification();
  statusLight_.setEnabled(false);
}

void HardwareDiagnosticsScreen::cyclePowerProfile() {
  const uint8_t next =
      (static_cast<uint8_t>(settings_.powerProfile()) + 1) %
      (static_cast<uint8_t>(PowerProfile::Performance) + 1);
  settings_.setPowerProfile(static_cast<PowerProfile>(next));
}

void HardwareDiagnosticsScreen::restoreAudio() {
  if (audioSilenceUntilMs_ == 0) return;
  M5.Speaker.setVolume(settings_.volume());
  audioSilenceUntilMs_ = 0;
}

void HardwareDiagnosticsScreen::restoreDisplay() {
  if (displayOffUntilMs_ != 0) {
    M5.Display.wakeup();
    displayOffUntilMs_ = 0;
  }
  if (brightnessTestUntilMs_ != 0 || brightnessTestLabel_ != nullptr) {
    M5.Display.setBrightness(settings_.activeBrightness());
    brightnessTestUntilMs_ = 0;
    brightnessTestLabel_ = nullptr;
  }
}

void HardwareDiagnosticsScreen::stopHaptic() {
  if (hapticUntilMs_ == 0) return;
  M5.Power.setVibration(0);
  hapticUntilMs_ = 0;
}

void HardwareDiagnosticsScreen::goBack() {
  restoreAudio();
  restoreDisplay();
  stopHaptic();
  if (manager_) manager_->show(ScreenId::Developer);
}

}  // namespace iris
