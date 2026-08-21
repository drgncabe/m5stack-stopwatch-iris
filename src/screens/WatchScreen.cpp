#include "iris/screens/WatchScreen.h"

#include <M5Unified.h>
#include "iris/Theme.h"
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
  previousWifi_ = "";
}

void WatchScreen::update(uint32_t nowMs) {
  const uint32_t intervalMs = settings_.lowPowerFace() ? 60000UL : 1000UL;
  if (lastDrawMs_ == 0 || nowMs - lastDrawMs_ >= intervalMs) {
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
  drawWifi();

  const Theme theme = currentTheme(settings_);
  if (!dt.valid) {
    if (previousValid_) {
      M5.Display.fillRect(46, 152, 374, 150, theme.background);
    }
    drawUnsetTime();
  } else {
    if (!previousValid_) {
      M5.Display.fillRect(46, 152, 374, 150, theme.background);
      previousMinute_ = -1;
      previousSecond_ = -1;
      previousDay_ = -1;
    }
    if (dt.hour * 100 + dt.minute != previousMinute_) {
      drawTime(dt);
    } else if (!settings_.lowPowerFace() && settings_.widgetEnabled(kWidgetSeconds) &&
               dt.second != previousSecond_) {
      M5.Display.fillRect(326, 184, 64, 42, theme.background);
      M5.Display.setFont(&fonts::FreeSans12pt7b);
      M5.Display.setTextDatum(middle_center);
      M5.Display.setTextColor(theme.accent, theme.background);
      char secondsText[8];
      snprintf(secondsText, sizeof(secondsText), ":%02d", dt.second);
      M5.Display.drawString(secondsText, 350, 206);
    }

    if (settings_.widgetEnabled(kWidgetDate) && dt.day != previousDay_) {
      drawDate(dt);
    } else if (!settings_.widgetEnabled(kWidgetDate) && previousDay_ != -2) {
      M5.Display.fillRect(50, 248, 366, 44, theme.background);
      previousDay_ = -2;
    }
  }

  previousValid_ = dt.valid;
  if (dt.valid) {
    previousMinute_ = dt.hour * 100 + dt.minute;
    previousSecond_ = dt.second;
    if (settings_.widgetEnabled(kWidgetDate)) {
      previousDay_ = dt.day;
    }
  }
}

void WatchScreen::drawStaticLayout() {
  const Theme theme = currentTheme(settings_);
  M5.Display.fillScreen(settings_.lowPowerFace() ? TFT_BLACK : theme.background);
  M5.Display.setTextDatum(middle_center);

  if (settings_.lowPowerFace()) return;

  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.drawString("IRIS", M5.Display.width() / 2, 72);

  M5.Display.setTextColor(theme.accent, theme.background);
  M5.Display.drawString("Tap or A for menu", M5.Display.width() / 2, 392);
}

void WatchScreen::drawBattery() {
  if (!settings_.widgetEnabled(kWidgetBattery)) {
    if (previousBattery_ != "") {
      M5.Display.fillRect(160, 16, 146, 44, settings_.lowPowerFace() ? TFT_BLACK : currentTheme(settings_).background);
      previousBattery_ = "";
    }
    return;
  }

  const String batteryText = battery_.statusText();
  if (batteryText == previousBattery_) return;

  const Theme theme = currentTheme(settings_);
  if (settings_.lowPowerFace()) {
    M5.Display.fillRect(172, 22, 122, 30, TFT_BLACK);
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextColor(theme.muted, TFT_BLACK);
    M5.Display.drawString(batteryText, M5.Display.width() / 2, 37);
    previousBattery_ = batteryText;
    return;
  }

  M5.Display.fillRoundRect(171, 20, 124, 34, 12, theme.panel);
  M5.Display.drawRoundRect(171, 20, 124, 34, 12, theme.accent);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(theme.foreground, theme.panel);
  M5.Display.drawString(batteryText, M5.Display.width() / 2, 37);
  previousBattery_ = batteryText;
}

void WatchScreen::drawWifi() {
  if (!settings_.widgetEnabled(kWidgetWifi) || settings_.lowPowerFace()) {
    if (previousWifi_ != "") {
      M5.Display.fillRect(54, 332, 358, 34, currentTheme(settings_).background);
      previousWifi_ = "";
    }
    return;
  }

  const String wifiText = wifi_.isConnected() ? String("WiFi ") + wifi_.ipAddress() : wifi_.statusText();
  if (wifiText == previousWifi_) return;

  const Theme theme = currentTheme(settings_);
  M5.Display.fillRect(54, 332, 358, 34, theme.background);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(theme.muted, theme.background);
  M5.Display.drawString(wifiText, M5.Display.width() / 2, 348);
  previousWifi_ = wifiText;
}

void WatchScreen::drawTime(const DateTimeSnapshot& dt) {
  const Theme theme = currentTheme(settings_);
  const uint16_t bg = settings_.lowPowerFace() ? TFT_BLACK : theme.timePanel;
  M5.Display.fillRect(60, 165, 350, 84, bg);
  M5.Display.setTextDatum(middle_center);

  char timeText[16];
  char secondsText[8];
  snprintf(timeText, sizeof(timeText), "%02d:%02d", dt.hour, dt.minute);
  snprintf(secondsText, sizeof(secondsText), ":%02d", dt.second);

  M5.Display.setFont(&fonts::FreeSansBold24pt7b);
  M5.Display.setTextColor(settings_.lowPowerFace() ? theme.muted : theme.foreground, bg);
  const int timeX = settings_.widgetEnabled(kWidgetSeconds) && !settings_.lowPowerFace()
                        ? M5.Display.width() / 2 - 12
                        : M5.Display.width() / 2;
  M5.Display.drawString(timeText, timeX, 198);

  if (settings_.lowPowerFace() || !settings_.widgetEnabled(kWidgetSeconds)) return;

  M5.Display.setFont(&fonts::FreeSans12pt7b);
  M5.Display.setTextColor(theme.accent, bg);
  M5.Display.drawString(secondsText, 350, 206);
}

void WatchScreen::drawDate(const DateTimeSnapshot& dt) {
  char dateText[32];
  snprintf(dateText, sizeof(dateText), "%s, %02d/%02d/%04d",
           kWeekDays[dt.weekDay % 7], dt.month, dt.day, dt.year);

  const Theme theme = currentTheme(settings_);
  const uint16_t bg = settings_.lowPowerFace() ? TFT_BLACK : theme.background;
  M5.Display.fillRect(50, 248, 366, 44, bg);
  M5.Display.setFont(&fonts::FreeSans12pt7b);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(settings_.lowPowerFace() ? theme.muted : theme.foreground, bg);
  M5.Display.drawString(dateText, M5.Display.width() / 2, 272);
}

void WatchScreen::drawUnsetTime() {
  const Theme theme = currentTheme(settings_);
  const uint16_t bg = settings_.lowPowerFace() ? TFT_BLACK : theme.background;
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.setTextColor(theme.foreground, bg);
  M5.Display.drawString("Time not set", M5.Display.width() / 2, 205);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(theme.accent, bg);
  M5.Display.drawString("Configure WiFi to sync", M5.Display.width() / 2, 250);
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
