#include "iris/screens/VolumeScreen.h"

#include <M5Unified.h>
#include "iris/Theme.h"
#include "iris/screens/ScreenManager.h"

namespace iris {

void VolumeScreen::enter() {}
void VolumeScreen::update(uint32_t) {}

void VolumeScreen::draw() {
  const Theme theme = currentTheme(settings_);
  M5.Display.fillScreen(theme.background);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.drawString("Volume", M5.Display.width() / 2, 62);

  drawVolumeValue();

  M5.Display.fillRoundRect(78, 265, 130, 70, 18, theme.button);
  M5.Display.fillRoundRect(258, 265, 130, 70, 18, theme.button);
  M5.Display.setTextColor(theme.foreground, theme.button);
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.drawString("-", 143, 300);
  M5.Display.drawString("+", 323, 300);

  M5.Display.fillRoundRect(138, 362, 190, 50, 16, theme.button);
  M5.Display.setFont(&fonts::FreeSans12pt7b);
  M5.Display.drawString("Back", M5.Display.width() / 2, 387);

  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(theme.muted, theme.background);
  M5.Display.drawString("A: Down     B: Up", M5.Display.width() / 2, 438);
}

void VolumeScreen::handleTouch(int32_t x, int32_t y) {
  if (y >= 255 && y <= 345) {
    if (x >= 60 && x <= 220) changeVolume(-16);
    if (x >= 246 && x <= 406) changeVolume(16);
  } else if (y >= 350 && y <= 425 && x >= 115 && x <= 350) {
    goBack();
  }
}

void VolumeScreen::onButtonA() { changeVolume(-16); }
void VolumeScreen::onButtonB() { changeVolume(16); }

void VolumeScreen::changeVolume(int delta) {
  int next = static_cast<int>(settings_.volume()) + delta;
  next = constrain(next, 0, 255);
  settings_.setVolume(static_cast<uint8_t>(next));
  M5.Speaker.setVolume(static_cast<uint8_t>(next));
  if (next > 0) M5.Speaker.tone(2800, 30);
  drawVolumeValue();
}

void VolumeScreen::drawVolumeValue() {
  const int volume = settings_.volume();
  const Theme theme = currentTheme(settings_);
  char value[16];
  snprintf(value, sizeof(value), "%d%%", (volume * 100) / 255);
  M5.Display.fillRect(110, 155, 246, 70, theme.background);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.setFont(&fonts::FreeSansBold24pt7b);
  M5.Display.drawString(value, M5.Display.width() / 2, 190);
}

void VolumeScreen::goBack() {
  if (manager_) manager_->show(ScreenId::Settings);
}

}  // namespace iris
