#include "iris/screens/BackgroundScreen.h"

#include <M5Unified.h>
#include "iris/screens/ScreenManager.h"

namespace iris {

void BackgroundScreen::enter() {}
void BackgroundScreen::update(uint32_t) {}

void BackgroundScreen::draw() {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.drawString("Background", M5.Display.width() / 2, 62);

  M5.Display.setFont(&fonts::FreeSansBold24pt7b);
  M5.Display.drawString(backgroundName(), M5.Display.width() / 2, 188);

  M5.Display.fillRoundRect(78, 280, 310, 62, 16, 0x2104);
  M5.Display.setFont(&fonts::FreeSans12pt7b);
  M5.Display.setTextColor(TFT_WHITE, 0x2104);
  M5.Display.drawString("Next", M5.Display.width() / 2, 311);

  M5.Display.fillRoundRect(138, 372, 190, 48, 16, 0x2104);
  M5.Display.drawString("Back", M5.Display.width() / 2, 396);

  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(0xBDF7, TFT_BLACK);
  M5.Display.drawString("A: Next     B: Back", M5.Display.width() / 2, 446);
}

void BackgroundScreen::handleTouch(int32_t x, int32_t y) {
  if (y >= 270 && y <= 352 && x >= 60 && x <= 406) {
    nextBackground();
  } else if (y >= 360 && y <= 430 && x >= 115 && x <= 350) {
    goBack();
  }
}

void BackgroundScreen::onButtonA() { nextBackground(); }
void BackgroundScreen::onButtonB() { goBack(); }

void BackgroundScreen::nextBackground() {
  settings_.setWatchBackground((settings_.watchBackground() + 1) % 5);
  draw();
}

void BackgroundScreen::goBack() {
  if (manager_) manager_->show(ScreenId::Settings);
}

const char* BackgroundScreen::backgroundName() const {
  switch (settings_.watchBackground() % 5) {
    case 1: return "Midnight";
    case 2: return "Forest";
    case 3: return "Plum";
    case 4: return "Steel";
    default: return "Black";
  }
}

}  // namespace iris
