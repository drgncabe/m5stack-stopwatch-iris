#include "iris/screens/DeviceInfoScreen.h"

#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>

#include "iris/AppConfig.h"
#include "iris/screens/ScreenManager.h"

namespace iris {

void DeviceInfoScreen::enter() {}
void DeviceInfoScreen::update(uint32_t) {}

void DeviceInfoScreen::draw() {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.drawString("Device Info", M5.Display.width() / 2, 50);

  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextDatum(top_left);

  int y = 96;
  const int x = 72;
  auto line = [&](const String& label, const String& value) {
    M5.Display.setTextColor(0xBDF7, TFT_BLACK);
    M5.Display.drawString(label, x, y);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.drawString(value, 190, y);
    y += 30;
  };

  line("Project", String(config::kProjectName));
  line("Version", String(config::kVersion));
  line("Chip", String(ESP.getChipModel()));
  line("CPU", String(ESP.getCpuFreqMHz()) + " MHz");
  line("Flash", String(ESP.getFlashChipSize() / (1024 * 1024)) + " MB");
  line("PSRAM", String(ESP.getPsramSize() / (1024 * 1024)) + " MB");
  line("Battery", battery_.statusText());
  line("RTC", timeService_.rtcAvailable() ? "Available" : "Unavailable");
  line("WiFi", wifi_.statusText());
  line("MAC", WiFi.macAddress());

  M5.Display.setTextDatum(middle_center);
  M5.Display.fillRoundRect(138, 392, 190, 48, 16, 0x2104);
  M5.Display.setTextColor(TFT_WHITE, 0x2104);
  M5.Display.drawString("Back", M5.Display.width() / 2, 416);
}

void DeviceInfoScreen::handleTouch(int32_t x, int32_t y) {
  if (y >= 380 && x >= 110 && x <= 355) goBack();
}

void DeviceInfoScreen::onButtonA() { goBack(); }
void DeviceInfoScreen::onButtonB() { goBack(); }

void DeviceInfoScreen::goBack() {
  if (manager_) manager_->show(ScreenId::Settings);
}

}  // namespace iris
