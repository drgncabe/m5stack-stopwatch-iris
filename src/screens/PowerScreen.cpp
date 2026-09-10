#include "iris/screens/PowerScreen.h"

#include <M5Unified.h>
#include "iris/Theme.h"
#include "iris/screens/ScreenManager.h"
#include "iris/screens/SettingsListRenderer.h"

namespace iris {

namespace {
constexpr size_t kItemCount = 11;
constexpr SettingsListLayout kListLayout = SettingsListRenderer::defaultLayout();

constexpr uint8_t kBrightnessValues[] = {48, 96, 160};
constexpr uint8_t kDimBrightnessValues[] = {8, 18, 32, 48};
constexpr uint16_t kDimTimeoutValues[] = {10, 20, 45, 120};
constexpr uint16_t kSleepTimeoutValues[] = {30, 90, 180, 300, 0};
constexpr uint16_t kTouchDelayValues[] = {95, 150, 225, 300};
}  // namespace

void PowerScreen::enter() {
  selected_ = 0;
}

void PowerScreen::update(uint32_t) {}

void PowerScreen::draw() {
  const Theme theme = currentTheme(settings_);
  M5.Display.fillScreen(theme.background);
  SettingsListRenderer::drawTitle(theme, "Power");

  for (size_t i = 0; i < kItemCount; ++i) {
    drawRow(i, i == selected_);
  }

  SettingsListRenderer::drawFooter(theme, "", "A: Next     B: Change", 428, 438);
}

void PowerScreen::previewTouch(int32_t x, int32_t y) {
  const int row = rowAt(x, y);
  if (row < 0) return;
  selectRow(static_cast<size_t>(row));
}

void PowerScreen::handleTouch(int32_t x, int32_t y) {
  const int row = rowAt(x, y);
  if (row < 0) return;
  selectRow(static_cast<size_t>(row));
  activateSelected();
}

void PowerScreen::onButtonA() {
  selectRow((selected_ + 1) % kItemCount);
}

void PowerScreen::onButtonB() {
  activateSelected();
}

void PowerScreen::activateSelected() {
  switch (selected_) {
    case 0: cycleBrightness(); break;
    case 1: cycleDimBrightness(); break;
    case 2: cycleDimTimeout(); break;
    case 3: cycleSleepTimeout(); break;
    case 4: cyclePowerProfile(); break;
    case 5: settings_.setWifiOnDemand(!settings_.wifiOnDemand()); break;
    case 6: settings_.setLowPowerFace(!settings_.lowPowerFace()); break;
    case 7: settings_.setAutoRotate(!settings_.autoRotate()); break;
    case 8: cycleTouchDelay(); break;
    case 9: settings_.setIndicatorLightEnabled(!settings_.indicatorLightEnabled()); break;
    default: goBack(); return;
  }
  drawRow(selected_, true);
}

void PowerScreen::selectRow(size_t index) {
  if (index >= kItemCount || index == selected_) return;
  const size_t previous = selected_;
  selected_ = index;
  drawRow(previous, false);
  drawRow(selected_, true);
}

void PowerScreen::drawRow(size_t index, bool selected) {
  if (index >= kItemCount) return;
  const char* label = "";
  String value;
  switch (index) {
    case 0:
      label = "Brightness";
      value = brightnessName();
      break;
    case 1:
      label = "Dim brightness";
      value = dimBrightnessName();
      break;
    case 2:
      label = "Dim";
      value = dimTimeoutName();
      break;
    case 3:
      label = "Sleep";
      value = sleepTimeoutName();
      break;
    case 4:
      label = "Power profile";
      value = powerProfileName(settings_.powerProfile());
      break;
    case 5:
      label = "WiFi on demand";
      value = settings_.wifiOnDemand() ? "On" : "Off";
      break;
    case 6:
      label = "Low-power face";
      value = settings_.lowPowerFace() ? "On" : "Off";
      break;
    case 7:
      label = "Auto rotate";
      value = settings_.autoRotate() ? "On" : "Off";
      break;
    case 8:
      label = "Touch delay";
      value = touchDelayName();
      break;
    case 9:
      label = "Indicator LED";
      value = settings_.indicatorLightEnabled() ? "On" : "Off";
      break;
    default:
      label = "Back";
      value = "";
      break;
  }

  SettingsListRenderer::drawRow(currentTheme(settings_), kListLayout, index, label, value, selected);
}

int PowerScreen::rowAt(int32_t x, int32_t y) const {
  return SettingsListRenderer::rowAt(kListLayout, kItemCount, x, y);
}

void PowerScreen::cycleBrightness() {
  uint8_t next = kBrightnessValues[0];
  for (size_t i = 0; i < sizeof(kBrightnessValues); ++i) {
    if (settings_.activeBrightness() <= kBrightnessValues[i]) {
      next = kBrightnessValues[(i + 1) % sizeof(kBrightnessValues)];
      break;
    }
  }
  settings_.setActiveBrightness(next);
  M5.Display.setBrightness(next);
}

void PowerScreen::cycleDimBrightness() {
  uint8_t next = kDimBrightnessValues[0];
  for (size_t i = 0; i < sizeof(kDimBrightnessValues); ++i) {
    if (settings_.dimBrightness() <= kDimBrightnessValues[i]) {
      next = kDimBrightnessValues[(i + 1) % sizeof(kDimBrightnessValues)];
      break;
    }
  }
  settings_.setDimBrightness(next);
}

void PowerScreen::cycleDimTimeout() {
  uint16_t next = kDimTimeoutValues[0];
  for (size_t i = 0; i < sizeof(kDimTimeoutValues) / sizeof(kDimTimeoutValues[0]); ++i) {
    if (settings_.dimTimeoutSeconds() == kDimTimeoutValues[i]) {
      next = kDimTimeoutValues[(i + 1) % (sizeof(kDimTimeoutValues) / sizeof(kDimTimeoutValues[0]))];
      break;
    }
  }
  settings_.setDimTimeoutSeconds(next);
}

void PowerScreen::cycleSleepTimeout() {
  uint16_t next = kSleepTimeoutValues[0];
  for (size_t i = 0; i < sizeof(kSleepTimeoutValues) / sizeof(kSleepTimeoutValues[0]); ++i) {
    if (settings_.sleepTimeoutSeconds() == kSleepTimeoutValues[i]) {
      next = kSleepTimeoutValues[(i + 1) % (sizeof(kSleepTimeoutValues) / sizeof(kSleepTimeoutValues[0]))];
      break;
    }
  }
  settings_.setSleepTimeoutSeconds(next);
}

void PowerScreen::cyclePowerProfile() {
  const uint8_t next =
      (static_cast<uint8_t>(settings_.powerProfile()) + 1) %
      (static_cast<uint8_t>(PowerProfile::Performance) + 1);
  settings_.setPowerProfile(static_cast<PowerProfile>(next));
}

void PowerScreen::cycleTouchDelay() {
  uint16_t next = kTouchDelayValues[0];
  for (size_t i = 0; i < sizeof(kTouchDelayValues) / sizeof(kTouchDelayValues[0]); ++i) {
    if (settings_.touchDelayMs() == kTouchDelayValues[i]) {
      next = kTouchDelayValues[(i + 1) % (sizeof(kTouchDelayValues) / sizeof(kTouchDelayValues[0]))];
      break;
    }
  }
  settings_.setTouchDelayMs(next);
}

const char* PowerScreen::brightnessName() const {
  if (settings_.activeBrightness() <= 48) return "Low";
  if (settings_.activeBrightness() <= 96) return "Med";
  return "High";
}

const char* PowerScreen::dimBrightnessName() const {
  if (settings_.dimBrightness() <= 8) return "Min";
  if (settings_.dimBrightness() <= 18) return "Low";
  if (settings_.dimBrightness() <= 32) return "Med";
  return "High";
}

const char* PowerScreen::dimTimeoutName() const {
  switch (settings_.dimTimeoutSeconds()) {
    case 10: return "10s";
    case 45: return "45s";
    case 120: return "2m";
    default: return "20s";
  }
}

const char* PowerScreen::sleepTimeoutName() const {
  switch (settings_.sleepTimeoutSeconds()) {
    case 0: return "Off";
    case 30: return "30s";
    case 180: return "3m";
    case 300: return "5m";
    default: return "90s";
  }
}

const char* PowerScreen::touchDelayName() const {
  if (settings_.touchDelayMs() <= 95) return "Fast";
  if (settings_.touchDelayMs() <= 150) return "Normal";
  if (settings_.touchDelayMs() <= 225) return "Calm";
  return "Slow";
}

void PowerScreen::goBack() {
  if (manager_) manager_->show(ScreenId::Settings);
}

}  // namespace iris
