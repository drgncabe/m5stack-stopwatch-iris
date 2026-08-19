#include "iris/screens/WatchScreen.h"

#include <M5Unified.h>
#include "iris/screens/ScreenManager.h"

namespace iris {

void WatchScreen::enter() {
  lastDrawMs_ = 0;
  layoutDrawn_ = false;
}

void WatchScreen::update(uint32_t nowMs) {
  if (lastDrawMs_ == 0 || nowMs - lastDrawMs_ >= 1000) {
    lastDrawMs_ = nowMs;
    draw();
  }
}

void WatchScreen::draw() {
  static constexpr const char* kWeekDays[] = {
      "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

  const DateTimeSnapshot dt = timeService_.now();

  if (!layoutDrawn_) {
    drawStaticLayout();
    layoutDrawn_ = true;
  }

  drawBattery();
  M5.Display.setTextDatum(middle_center);
  M5.Display.fillRect(46, 152, 374, 150, TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);

  if (!dt.valid) {
    M5.Display.setFont(&fonts::FreeSansBold18pt7b);
    M5.Display.drawString("Time not set", M5.Display.width() / 2, 205);
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.setTextColor(0xBDF7, TFT_BLACK);
    M5.Display.drawString("Configure WiFi to sync", M5.Display.width() / 2, 250);
  } else {
    char timeText[16];
    char secondsText[8];
    char dateText[32];
    snprintf(timeText, sizeof(timeText), "%02d:%02d", dt.hour, dt.minute);
    snprintf(secondsText, sizeof(secondsText), ":%02d", dt.second);
    snprintf(dateText, sizeof(dateText), "%s, %02d/%02d/%04d",
             kWeekDays[dt.weekDay % 7], dt.month, dt.day, dt.year);

    M5.Display.setFont(&fonts::FreeSansBold24pt7b);
    M5.Display.drawString(timeText, M5.Display.width() / 2 - 12, 198);
    M5.Display.setFont(&fonts::FreeSans12pt7b);
    M5.Display.setTextColor(0xBDF7, TFT_BLACK);
    M5.Display.drawString(secondsText, 350, 206);

    M5.Display.setFont(&fonts::FreeSans12pt7b);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.drawString(dateText, M5.Display.width() / 2, 272);
  }

  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(0xBDF7, TFT_BLACK);
  M5.Display.drawString("Tap or A for menu", M5.Display.width() / 2, 392);
}

void WatchScreen::drawStaticLayout() {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextDatum(middle_center);

  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.drawString("IRIS", M5.Display.width() / 2, 72);

  M5.Display.setTextColor(0xBDF7, TFT_BLACK);
  M5.Display.drawString("Tap or A for menu", M5.Display.width() / 2, 392);
}

void WatchScreen::drawBattery() {
  M5.Display.fillRect(328, 58, 92, 28, TFT_BLACK);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextDatum(middle_right);
  M5.Display.setTextColor(0xBDF7, TFT_BLACK);
  M5.Display.drawString(battery_.statusText(), 410, 72);
}

void WatchScreen::handleTouch(int32_t, int32_t) {
  if (manager_) manager_->show(ScreenId::MainMenu);
}

void WatchScreen::onButtonA() {
  if (manager_) manager_->show(ScreenId::MainMenu);
}

void WatchScreen::onButtonB() {
  // Keep BtnB useful as a lightweight hardware test while preserving the
  // stock M5Stack serial logging in the application input loop.
  M5.Speaker.tone(4000, 20);
}

}  // namespace iris
