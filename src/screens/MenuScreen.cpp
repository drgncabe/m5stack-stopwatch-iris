#include "iris/screens/MenuScreen.h"

#include <M5Unified.h>
#include "iris/Theme.h"
#include "iris/screens/ScreenManager.h"

namespace iris {

namespace {
constexpr int kRowHeight = 46;
constexpr int kRowStartY = 76;
constexpr int kRowLeft = 58;
constexpr int kRowWidth = 350;
constexpr int kRowRectHeight = 40;
}  // namespace

MenuScreen::MenuScreen(const char* title, const MenuItem* items, size_t itemCount,
                       SettingsStore& settings)
    : title_(title), items_(items), itemCount_(itemCount), settings_(settings) {}

void MenuScreen::enter() {
  selected_ = 0;
}

void MenuScreen::update(uint32_t) {}

void MenuScreen::draw() {
  const Theme theme = currentTheme(settings_);
  M5.Display.fillScreen(theme.background);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.drawString(title_, M5.Display.width() / 2, 52);

  for (size_t i = 0; i < itemCount_; ++i) {
    drawRow(i, i == selected_);
  }

  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(theme.muted, theme.background);
  M5.Display.drawString("A: Next     B: Select", M5.Display.width() / 2, 426);
}

void MenuScreen::handleTouch(int32_t x, int32_t y) {
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

void MenuScreen::onButtonA() {
  if (itemCount_ == 0) return;
  const size_t previous = selected_;
  selected_ = (selected_ + 1) % itemCount_;
  drawRow(previous, false);
  drawRow(selected_, true);
}

void MenuScreen::onButtonB() {
  activateSelected();
}

void MenuScreen::activateSelected() {
  if (!manager_ || selected_ >= itemCount_) return;
  manager_->show(items_[selected_].target);
}

void MenuScreen::drawRow(size_t index, bool selected) {
  if (index >= itemCount_) return;
  const Theme theme = currentTheme(settings_);
  const int y = kRowStartY + static_cast<int>(index) * kRowHeight;
  const uint16_t fill = selected ? theme.selected : theme.background;
  const uint16_t border = selected ? theme.foreground : theme.panel;

  M5.Display.fillRoundRect(kRowLeft, y, kRowWidth, kRowRectHeight, 14, fill);
  M5.Display.drawRoundRect(kRowLeft, y, kRowWidth, kRowRectHeight, 14, border);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSans12pt7b);
  M5.Display.setTextColor(theme.foreground, fill);
  M5.Display.drawString(items_[index].label, M5.Display.width() / 2, y + (kRowRectHeight / 2));
}

int MenuScreen::rowAt(int32_t x, int32_t y) const {
  if (x < kRowLeft || x > kRowLeft + kRowWidth) return -1;
  for (size_t i = 0; i < itemCount_; ++i) {
    const int rowY = kRowStartY + static_cast<int>(i) * kRowHeight;
    if (y >= rowY && y <= rowY + kRowRectHeight) return static_cast<int>(i);
  }
  return -1;
}

}  // namespace iris
