#include "iris/screens/HardwareDiagnosticsScreen.h"

#include <M5Unified.h>
#include <WiFi.h>

#include "iris/Theme.h"
#include "iris/screens/ScreenManager.h"

namespace iris {

namespace {
constexpr int kRowHeight = 34;
constexpr int kRowStartY = 76;
constexpr int kRowLeft = 38;
constexpr int kRowWidth = 390;
constexpr int kRowRectHeight = 29;
constexpr size_t kItemCount = 10;
constexpr uint32_t kShortTestMs = 5000;
constexpr uint32_t kHapticTestMs = 650;
constexpr uint32_t kStatusRefreshMs = 500;

bool timeExpired(uint32_t nowMs, uint32_t untilMs) {
  return untilMs != 0 && static_cast<int32_t>(nowMs - untilMs) >= 0;
}
}  // namespace

void HardwareDiagnosticsScreen::enter() {
  selected_ = 0;
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

  if (shouldRedraw || nowMs - lastStatusDrawMs_ >= kStatusRefreshMs) {
    drawFooter();
    drawRow(selected_, true);
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
  const int row = rowAt(x, y);
  if (row < 0) return;
  selectRow(static_cast<size_t>(row));
}

void HardwareDiagnosticsScreen::handleTouch(int32_t x, int32_t y) {
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
  switch (selected_) {
    case 0: startAudioSilence(nowMs); break;
    case 1: testAudioTone(); break;
    case 2: startDisplayOff(nowMs); break;
    case 3: startBrightnessTest(16, "Min test", nowMs); break;
    case 4: startBrightnessTest(200, "High test", nowMs); break;
    case 5: toggleWifi(); break;
    case 6: toggleWifiSleep(); break;
    case 7: startHapticPulse(nowMs); break;
    case 8: cyclePowerProfile(); break;
    default: goBack(); return;
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

void HardwareDiagnosticsScreen::drawRow(size_t index, bool selected) {
  if (index >= kItemCount || displayOffUntilMs_ != 0) return;

  const Theme theme = currentTheme(settings_);
  const int y = kRowStartY + static_cast<int>(index) * kRowHeight;
  const uint16_t fill = selected ? theme.selected : theme.background;
  const uint16_t border = selected ? theme.foreground : theme.panel;

  const char* label = "";
  String value;
  switch (index) {
    case 0:
      label = "Audio silence";
      value = audioSilenceUntilMs_ == 0 ? "Ready" : "Muted";
      break;
    case 1:
      label = "Audio tone";
      value = "Tap";
      break;
    case 2:
      label = "Display off";
      value = displayOffUntilMs_ == 0 ? "5 sec" : "Off";
      break;
    case 3:
      label = "Brightness min";
      value = brightnessTestLabel_ == nullptr ? "5 sec" : brightnessTestLabel_;
      break;
    case 4:
      label = "Brightness high";
      value = brightnessTestLabel_ == nullptr ? "5 sec" : brightnessTestLabel_;
      break;
    case 5:
      label = "WiFi radio";
      value = wifi_.isEnabled() ? "On" : "Off";
      break;
    case 6:
      label = "WiFi sleep";
      value = wifiSleep_ ? "On" : "Off";
      break;
    case 7:
      label = "Haptic pulse";
      value = hapticUntilMs_ == 0 ? "Ready" : "On";
      break;
    case 8:
      label = "Power profile";
      value = powerProfileName(settings_.powerProfile());
      break;
    default:
      label = "Back";
      break;
  }

  M5.Display.fillRoundRect(kRowLeft, y, kRowWidth, kRowRectHeight, 10, fill);
  M5.Display.drawRoundRect(kRowLeft, y, kRowWidth, kRowRectHeight, 10, border);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextDatum(middle_left);
  M5.Display.setTextColor(theme.foreground, fill);
  M5.Display.drawString(label, kRowLeft + 16, y + (kRowRectHeight / 2));
  M5.Display.setTextDatum(middle_right);
  M5.Display.setTextColor(theme.muted, fill);
  M5.Display.drawString(value, kRowLeft + kRowWidth - 16, y + (kRowRectHeight / 2));
}

void HardwareDiagnosticsScreen::drawFooter() {
  if (displayOffUntilMs_ != 0) return;

  const Theme theme = currentTheme(settings_);
  M5.Display.fillRect(18, 418, 430, 36, theme.background);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(theme.muted, theme.background);
  String status = "CPU ";
  status += String(getCpuFrequencyMhz());
  status += " MHz  ";
  status += wifi_.statusText();
  M5.Display.drawString(status, M5.Display.width() / 2, 428);
  M5.Display.drawString("A: Next     B: Run", M5.Display.width() / 2, 448);
}

int HardwareDiagnosticsScreen::rowAt(int32_t x, int32_t y) const {
  if (x < kRowLeft || x > kRowLeft + kRowWidth) return -1;
  for (size_t i = 0; i < kItemCount; ++i) {
    const int rowY = kRowStartY + static_cast<int>(i) * kRowHeight;
    if (y >= rowY - 3 && y <= rowY + kRowRectHeight + 3) return static_cast<int>(i);
  }
  return -1;
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
