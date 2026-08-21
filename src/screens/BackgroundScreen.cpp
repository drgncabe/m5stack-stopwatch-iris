#include "iris/screens/BackgroundScreen.h"

#include <M5Unified.h>
#include "iris/Theme.h"
#include "iris/screens/ScreenManager.h"

namespace iris {

namespace {
constexpr int kRowHeight = 47;
constexpr int kRowStartY = 84;
constexpr int kRowLeft = 42;
constexpr int kRowWidth = 382;
constexpr int kRowRectHeight = 40;
constexpr size_t kItemCount = 6;
}  // namespace

void BackgroundScreen::enter() {
  selected_ = 0;
}

void BackgroundScreen::update(uint32_t) {}

void BackgroundScreen::draw() {
  const Theme theme = currentTheme(settings_);
  M5.Display.fillScreen(theme.background);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.drawString("Theme", M5.Display.width() / 2, 50);

  for (size_t i = 0; i < kItemCount; ++i) {
    drawRow(i, i == selected_);
  }

  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(theme.muted, theme.background);
  M5.Display.drawString("A: Next     B: Change", M5.Display.width() / 2, 438);
}

void BackgroundScreen::previewTouch(int32_t x, int32_t y) {
  const int row = rowAt(x, y);
  if (row < 0) return;
  selectRow(static_cast<size_t>(row));
}

void BackgroundScreen::handleTouch(int32_t x, int32_t y) {
  const int row = rowAt(x, y);
  if (row < 0) return;
  selectRow(static_cast<size_t>(row));
  activateSelected();
}

void BackgroundScreen::onButtonA() {
  selectRow((selected_ + 1) % kItemCount);
}

void BackgroundScreen::onButtonB() { activateSelected(); }

void BackgroundScreen::activateSelected() {
  switch (selected_) {
    case 0: nextTheme(); break;
    case 1: toggleWidget(kWidgetBattery); break;
    case 2: toggleWidget(kWidgetDate); break;
    case 3: toggleWidget(kWidgetSeconds); break;
    case 4: toggleWidget(kWidgetWifi); break;
    default: goBack(); return;
  }
  draw();
}

void BackgroundScreen::selectRow(size_t index) {
  if (index >= kItemCount || index == selected_) return;
  const size_t previous = selected_;
  selected_ = index;
  drawRow(previous, false);
  drawRow(selected_, true);
}

void BackgroundScreen::drawRow(size_t index, bool selected) {
  if (index >= kItemCount) return;
  const Theme theme = currentTheme(settings_);
  const int y = kRowStartY + static_cast<int>(index) * kRowHeight;
  const uint16_t fill = selected ? theme.selected : theme.background;
  const uint16_t border = selected ? theme.foreground : theme.panel;

  M5.Display.fillRoundRect(kRowLeft, y, kRowWidth, kRowRectHeight, 14, fill);
  M5.Display.drawRoundRect(kRowLeft, y, kRowWidth, kRowRectHeight, 14, border);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(theme.foreground, fill);
  M5.Display.setTextDatum(middle_left);
  M5.Display.drawString(rowLabel(index), kRowLeft + 18, y + (kRowRectHeight / 2));
  M5.Display.setTextDatum(middle_right);
  M5.Display.setTextColor(theme.muted, fill);
  M5.Display.drawString(rowValue(index), kRowLeft + kRowWidth - 18, y + (kRowRectHeight / 2));
}

int BackgroundScreen::rowAt(int32_t x, int32_t y) const {
  if (x < kRowLeft || x > kRowLeft + kRowWidth) return -1;
  for (size_t i = 0; i < kItemCount; ++i) {
    const int rowY = kRowStartY + static_cast<int>(i) * kRowHeight;
    if (y >= rowY - 3 && y <= rowY + kRowRectHeight + 3) return static_cast<int>(i);
  }
  return -1;
}

void BackgroundScreen::nextTheme() {
  settings_.setThemeId((settings_.themeId() + 1) % kThemeCount);
}

void BackgroundScreen::toggleWidget(uint8_t widget) {
  settings_.setWidgetEnabled(widget, !settings_.widgetEnabled(widget));
}

void BackgroundScreen::goBack() {
  if (manager_) manager_->show(ScreenId::Settings);
}

const char* BackgroundScreen::rowLabel(size_t index) const {
  switch (index) {
    case 0: return "Theme";
    case 1: return "Battery";
    case 2: return "Date";
    case 3: return "Seconds";
    case 4: return "WiFi";
    default: return "Back";
  }
}

String BackgroundScreen::rowValue(size_t index) const {
  switch (index) {
    case 0: return themeName(settings_);
    case 1: return settings_.widgetEnabled(kWidgetBattery) ? "On" : "Off";
    case 2: return settings_.widgetEnabled(kWidgetDate) ? "On" : "Off";
    case 3: return settings_.widgetEnabled(kWidgetSeconds) ? "On" : "Off";
    case 4: return settings_.widgetEnabled(kWidgetWifi) ? "On" : "Off";
    default: return "";
  }
}

}  // namespace iris
