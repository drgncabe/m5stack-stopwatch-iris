#include "iris/screens/MenuScreen.h"

#include <M5Unified.h>
#include "iris/screens/ScreenManager.h"

namespace iris {

namespace {
constexpr int kRowHeight = 64;
constexpr int kRowStartY = 92;
constexpr int kRowLeft = 58;
constexpr int kRowWidth = 350;
}

MenuScreen::MenuScreen(const char* title, const MenuItem* items, size_t itemCount)
    : title_(title), items_(items), itemCount_(itemCount) {}

void MenuScreen::enter() {
  selected_ = 0;
}

void MenuScreen::update(uint32_t) {}

void MenuScreen::draw() {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.drawString(title_, M5.Display.width() / 2, 52);

  M5.Display.setFont(&fonts::FreeSans12pt7b);
  for (size_t i = 0; i < itemCount_; ++i) {
    const int y = kRowStartY + static_cast<int>(i) * kRowHeight;
    const uint16_t fill = (i == selected_) ? TFT_DARKGREY : TFT_BLACK;
    const uint16_t border = (i == selected_) ? TFT_WHITE : 0x4208;
    M5.Display.fillRoundRect(kRowLeft, y, kRowWidth, 50, 16, fill);
    M5.Display.drawRoundRect(kRowLeft, y, kRowWidth, 50, 16, border);
    M5.Display.setTextColor(TFT_WHITE, fill);
    M5.Display.drawString(items_[i].label, M5.Display.width() / 2, y + 25);
  }

  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(0xBDF7, TFT_BLACK);
  M5.Display.drawString("A: Next     B: Select", M5.Display.width() / 2, 414);
}

void MenuScreen::handleTouch(int32_t x, int32_t y) {
  const int row = rowAt(x, y);
  if (row < 0) return;
  selected_ = static_cast<size_t>(row);
  draw();
  activateSelected();
}

void MenuScreen::onButtonA() {
  if (itemCount_ == 0) return;
  selected_ = (selected_ + 1) % itemCount_;
  draw();
}

void MenuScreen::onButtonB() {
  activateSelected();
}

void MenuScreen::activateSelected() {
  if (!manager_ || selected_ >= itemCount_) return;
  manager_->show(items_[selected_].target);
}

int MenuScreen::rowAt(int32_t x, int32_t y) const {
  if (x < kRowLeft || x > kRowLeft + kRowWidth) return -1;
  for (size_t i = 0; i < itemCount_; ++i) {
    const int rowY = kRowStartY + static_cast<int>(i) * kRowHeight;
    if (y >= rowY && y <= rowY + 50) return static_cast<int>(i);
  }
  return -1;
}

}  // namespace iris
