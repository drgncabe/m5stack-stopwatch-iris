#include "iris/screens/PowerScreen.h"

#include <M5Unified.h>
#include "iris/Theme.h"
#include "iris/screens/ScreenManager.h"

namespace iris {

namespace {
constexpr int kRowHeight = 54;
constexpr int kRowStartY = 88;
constexpr int kRowLeft = 42;
constexpr int kRowWidth = 382;
constexpr size_t kItemCount = 6;

constexpr uint8_t kBrightnessValues[] = {48, 96, 160};
constexpr uint16_t kDimTimeoutValues[] = {10, 20, 45, 120};
constexpr uint16_t kSleepTimeoutValues[] = {30, 90, 180, 300, 0};
}  // namespace

void PowerScreen::enter() {
  selected_ = 0;
}

void PowerScreen::update(uint32_t) {}

void PowerScreen::draw() {
  const Theme theme = currentTheme(settings_);
  M5.Display.fillScreen(theme.background);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.drawString("Power", M5.Display.width() / 2, 50);

  for (size_t i = 0; i < kItemCount; ++i) {
    drawRow(i, i == selected_);
  }

  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(theme.muted, theme.background);
  M5.Display.drawString("A: Next     B: Change", M5.Display.width() / 2, 438);
}

void PowerScreen::handleTouch(int32_t x, int32_t y) {
  const int row = rowAt(x, y);
  if (row < 0) return;
  if (static_cast<size_t>(row) != selected_) {
    const size_t previous = selected_;
    selected_ = static_cast<size_t>(row);
    drawRow(previous, false);
    drawRow(selected_, true);
  }
  activateSelected();
}

void PowerScreen::onButtonA() {
  const size_t previous = selected_;
  selected_ = (selected_ + 1) % kItemCount;
  drawRow(previous, false);
  drawRow(selected_, true);
}

void PowerScreen::onButtonB() {
  activateSelected();
}

void PowerScreen::activateSelected() {
  switch (selected_) {
    case 0: cycleBrightness(); break;
    case 1: cycleDimTimeout(); break;
    case 2: cycleSleepTimeout(); break;
    case 3: settings_.setWifiOnDemand(!settings_.wifiOnDemand()); break;
    case 4: settings_.setLowPowerFace(!settings_.lowPowerFace()); break;
    default: goBack(); return;
  }
  draw();
}

void PowerScreen::drawRow(size_t index, bool selected) {
  if (index >= kItemCount) return;
  const Theme theme = currentTheme(settings_);
  const int y = kRowStartY + static_cast<int>(index) * kRowHeight;
  const uint16_t fill = selected ? theme.selected : theme.background;
  const uint16_t border = selected ? theme.foreground : theme.panel;

  const char* label = "";
  String value;
  switch (index) {
    case 0:
      label = "Brightness";
      value = brightnessName();
      break;
    case 1:
      label = "Dim";
      value = dimTimeoutName();
      break;
    case 2:
      label = "Sleep";
      value = sleepTimeoutName();
      break;
    case 3:
      label = "WiFi on demand";
      value = settings_.wifiOnDemand() ? "On" : "Off";
      break;
    case 4:
      label = "Low-power face";
      value = settings_.lowPowerFace() ? "On" : "Off";
      break;
    default:
      label = "Back";
      value = "";
      break;
  }

  M5.Display.fillRoundRect(kRowLeft, y, kRowWidth, 44, 14, fill);
  M5.Display.drawRoundRect(kRowLeft, y, kRowWidth, 44, 14, border);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(theme.foreground, fill);
  M5.Display.setTextDatum(middle_left);
  M5.Display.drawString(label, kRowLeft + 18, y + 22);
  M5.Display.setTextDatum(middle_right);
  M5.Display.setTextColor(theme.muted, fill);
  M5.Display.drawString(value, kRowLeft + kRowWidth - 18, y + 22);
}

int PowerScreen::rowAt(int32_t x, int32_t y) const {
  if (x < kRowLeft || x > kRowLeft + kRowWidth) return -1;
  for (size_t i = 0; i < kItemCount; ++i) {
    const int rowY = kRowStartY + static_cast<int>(i) * kRowHeight;
    if (y >= rowY && y <= rowY + 44) return static_cast<int>(i);
  }
  return -1;
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

const char* PowerScreen::brightnessName() const {
  if (settings_.activeBrightness() <= 48) return "Low";
  if (settings_.activeBrightness() <= 96) return "Med";
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

void PowerScreen::goBack() {
  if (manager_) manager_->show(ScreenId::Settings);
}

}  // namespace iris
