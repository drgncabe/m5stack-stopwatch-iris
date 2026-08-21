#include "iris/screens/MenuScreen.h"

#include <M5Unified.h>
#include "iris/Theme.h"
#include "iris/screens/ScreenManager.h"

namespace iris {

namespace {
constexpr int kTitleY = 50;
constexpr int kListCenterY = 240;
constexpr int kRowHeight = 52;
constexpr int kRowLeft = 52;
constexpr int kRowWidth = 362;
constexpr int kRowRectHeight = 44;
constexpr int kRowHitPaddingX = 34;
constexpr int kVisiblePaddingRows = 5;
constexpr uint32_t kHapticPulseMs = 14;
}  // namespace

MenuScreen::MenuScreen(const char* title, const MenuItem* items, size_t itemCount,
                       SettingsStore& settings)
    : title_(title), items_(items), itemCount_(itemCount), settings_(settings) {}

void MenuScreen::enter() {
  selected_ = 0;
  touchMovedSelection_ = false;
  stopSelectionHaptic();
}

void MenuScreen::update(uint32_t nowMs) {
  if (hapticActive_ && nowMs >= hapticOffMs_) {
    stopSelectionHaptic();
  }
}

void MenuScreen::draw() {
  const Theme theme = currentTheme(settings_);
  M5.Display.fillScreen(theme.background);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.drawString(title_, M5.Display.width() / 2, kTitleY);

  for (size_t i = 0; i < itemCount_; ++i) {
    drawRow(i, i == selected_);
  }

  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(theme.muted, theme.background);
  M5.Display.drawString("A: Next     B: Select", M5.Display.width() / 2, 426);
}

void MenuScreen::previewTouch(int32_t x, int32_t y) {
  const int row = rowAt(x, y);
  if (row < 0) return;
  if (static_cast<size_t>(row) != selected_) touchMovedSelection_ = true;
  selectRow(static_cast<size_t>(row));
}

void MenuScreen::handleTouch(int32_t x, int32_t y) {
  const int row = rowAt(x, y);
  if (row < 0) return;
  selectRow(static_cast<size_t>(row));
  if (touchMovedSelection_) {
    touchMovedSelection_ = false;
    return;
  }
  activateSelected();
}

void MenuScreen::onButtonA() {
  if (itemCount_ == 0) return;
  selectRow((selected_ + 1) % itemCount_);
}

void MenuScreen::onButtonB() {
  activateSelected();
}

void MenuScreen::activateSelected() {
  if (!manager_ || selected_ >= itemCount_) return;
  manager_->show(items_[selected_].target);
}

void MenuScreen::selectRow(size_t row) {
  if (row >= itemCount_ || row == selected_) return;
  selected_ = row;
  draw();
  startSelectionHaptic();
}

void MenuScreen::drawRow(size_t index, bool selected) {
  if (index >= itemCount_) return;
  const Theme theme = currentTheme(settings_);
  const int offset = static_cast<int>(index) - static_cast<int>(selected_);
  if (abs(offset) > kVisiblePaddingRows) return;

  const int y = kListCenterY + offset * kRowHeight - (kRowRectHeight / 2);
  if (y < 72 || y > 406) return;

  const int distance = abs(offset);
  const uint16_t fill = selected ? theme.selected : theme.background;
  const uint16_t text = selected ? theme.foreground : (distance > 1 ? theme.muted : theme.foreground);
  const int inset = selected ? 0 : distance * 12;
  const int rowX = kRowLeft + inset;
  const int rowW = kRowWidth - (inset * 2);

  if (selected) {
    M5.Display.fillRoundRect(rowX, y, rowW, kRowRectHeight, 18, fill);
  }
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(selected ? &fonts::FreeSansBold12pt7b : &fonts::FreeSans12pt7b);
  M5.Display.setTextColor(text, fill);
  M5.Display.drawString(items_[index].label, M5.Display.width() / 2, y + (kRowRectHeight / 2));
}

int MenuScreen::rowAt(int32_t x, int32_t y) const {
  if (x < kRowLeft - kRowHitPaddingX || x > kRowLeft + kRowWidth + kRowHitPaddingX) return -1;
  if (itemCount_ == 0) return -1;

  if (y < 76 || y > 404) return -1;

  int row = static_cast<int>(selected_) + (y - kListCenterY + (kRowHeight / 2)) / kRowHeight;
  row = constrain(row, 0, static_cast<int>(itemCount_) - 1);
  return row;
}

void MenuScreen::startSelectionHaptic() {
  M5.Power.setVibration(82);
  hapticActive_ = true;
  hapticOffMs_ = millis() + kHapticPulseMs;
}

void MenuScreen::stopSelectionHaptic() {
  if (!hapticActive_) return;
  M5.Power.setVibration(0);
  hapticActive_ = false;
}

}  // namespace iris
