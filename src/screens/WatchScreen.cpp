#include "iris/screens/WatchScreen.h"

#include <M5Unified.h>
#include "iris/screens/ScreenManager.h"

namespace iris {

namespace {
constexpr const char* kWeekDays[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
}

void WatchScreen::enter() {
  lastDrawMs_ = 0;
  layoutDrawn_ = false;
  previousValid_ = false;
  previousMinute_ = -1;
  previousSecond_ = -1;
  previousDay_ = -1;
  previousBattery_ = "";
}

void WatchScreen::update(uint32_t nowMs) {
  if (lastDrawMs_ == 0 || nowMs - lastDrawMs_ >= 1000) {
    lastDrawMs_ = nowMs;
    draw();
  }
}

void WatchScreen::draw() {
  const DateTimeSnapshot dt = timeService_.now();

  if (!layoutDrawn_) {
    drawStaticLayout();
    layoutDrawn_ = true;
  }

  drawBattery();

  if (!dt.valid) {
    if (previousValid_) {
      M5.Display.fillRect(46, 152, 374, 150, backgroundColor());
    }
    drawUnsetTime();
  } else {
    if (!previousValid_) {
      M5.Display.fillRect(46, 152, 374, 150, backgroundColor());
      previousMinute_ = -1;
      previousSecond_ = -1;
      previousDay_ = -1;
    }
    if (dt.hour * 100 + dt.minute != previousMinute_) {
      drawTime(dt);
    } else if (dt.second != previousSecond_) {
      M5.Display.fillRect(326, 184, 64, 42, backgroundColor());
      M5.Display.setFont(&fonts::FreeSans12pt7b);
      M5.Display.setTextDatum(middle_center);
      M5.Display.setTextColor(accentColor(), backgroundColor());
      char secondsText[8];
      snprintf(secondsText, sizeof(secondsText), ":%02d", dt.second);
      M5.Display.drawString(secondsText, 350, 206);
    }

    if (dt.day != previousDay_) {
      drawDate(dt);
    }
  }

  previousValid_ = dt.valid;
  if (dt.valid) {
    previousMinute_ = dt.hour * 100 + dt.minute;
    previousSecond_ = dt.second;
    previousDay_ = dt.day;
  }
}

void WatchScreen::drawStaticLayout() {
  M5.Display.fillScreen(backgroundColor());
  M5.Display.setTextDatum(middle_center);

  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(TFT_WHITE, backgroundColor());
  M5.Display.drawString("IRIS", M5.Display.width() / 2, 72);

  M5.Display.setTextColor(accentColor(), backgroundColor());
  M5.Display.drawString("Tap or A for menu", M5.Display.width() / 2, 392);
}

void WatchScreen::drawBattery() {
  const String batteryText = battery_.statusText();
  if (batteryText == previousBattery_) return;

  M5.Display.fillRect(300, 58, 120, 28, backgroundColor());
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextDatum(middle_right);
  M5.Display.setTextColor(accentColor(), backgroundColor());
  M5.Display.drawString(batteryText, 410, 72);
  previousBattery_ = batteryText;
}

void WatchScreen::drawTime(const DateTimeSnapshot& dt) {
  M5.Display.fillRect(74, 170, 300, 70, backgroundColor());
  M5.Display.setTextDatum(middle_center);

  char timeText[16];
  char secondsText[8];
  snprintf(timeText, sizeof(timeText), "%02d:%02d", dt.hour, dt.minute);
  snprintf(secondsText, sizeof(secondsText), ":%02d", dt.second);

  M5.Display.setFont(&fonts::FreeSansBold24pt7b);
  M5.Display.setTextColor(TFT_WHITE, backgroundColor());
  M5.Display.drawString(timeText, M5.Display.width() / 2 - 12, 198);

  M5.Display.setFont(&fonts::FreeSans12pt7b);
  M5.Display.setTextColor(accentColor(), backgroundColor());
  M5.Display.drawString(secondsText, 350, 206);
}

void WatchScreen::drawDate(const DateTimeSnapshot& dt) {
  char dateText[32];
  snprintf(dateText, sizeof(dateText), "%s, %02d/%02d/%04d",
           kWeekDays[dt.weekDay % 7], dt.month, dt.day, dt.year);

  M5.Display.fillRect(50, 248, 366, 44, backgroundColor());
  M5.Display.setFont(&fonts::FreeSans12pt7b);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(TFT_WHITE, backgroundColor());
  M5.Display.drawString(dateText, M5.Display.width() / 2, 272);
}

void WatchScreen::drawUnsetTime() {
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.setTextColor(TFT_WHITE, backgroundColor());
  M5.Display.drawString("Time not set", M5.Display.width() / 2, 205);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(accentColor(), backgroundColor());
  M5.Display.drawString("Configure WiFi to sync", M5.Display.width() / 2, 250);
}

uint16_t WatchScreen::backgroundColor() const {
  switch (settings_.watchBackground() % 5) {
    case 1: return 0x0010;
    case 2: return 0x0188;
    case 3: return 0x2008;
    case 4: return 0x2104;
    default: return TFT_BLACK;
  }
}

uint16_t WatchScreen::accentColor() const {
  switch (settings_.watchBackground() % 5) {
    case 1: return 0x867F;
    case 2: return 0xB7E0;
    case 3: return 0xFBBF;
    case 4: return 0xD6BA;
    default: return 0xBDF7;
  }
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
