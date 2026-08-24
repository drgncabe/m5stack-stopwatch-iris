#include "iris/screens/DeviceInfoScreen.h"

#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>

#include "iris/AppConfig.h"
#include "iris/Theme.h"
#include "iris/screens/ScreenManager.h"

namespace iris {

namespace {
String formatBytes(uint32_t bytes) {
  if (bytes >= 1024UL * 1024UL) {
    return String(bytes / (1024UL * 1024UL)) + "." +
           String((bytes % (1024UL * 1024UL)) / 104858UL) + " MB";
  }
  if (bytes >= 1024UL) return String(bytes / 1024UL) + " KB";
  return String(bytes) + " B";
}
}  // namespace

void DeviceInfoScreen::enter() {}
void DeviceInfoScreen::update(uint32_t) {}

void DeviceInfoScreen::draw() {
  const Theme theme = currentTheme(settings_);
  M5.Display.fillScreen(theme.background);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.drawString("Device Info", M5.Display.width() / 2, 50);

  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextDatum(top_left);

  int y = 82;
  const int x = 58;
  auto line = [&](const String& label, const String& value) {
    M5.Display.setTextColor(theme.muted, theme.background);
    M5.Display.drawString(label, x, y);
    M5.Display.setTextColor(theme.foreground, theme.background);
    M5.Display.drawString(value, 184, y);
    y += 24;
  };

  line("Project", String(config::kProjectName));
  line("Version", String(config::kVersion));
  line("Chip", String(ESP.getChipModel()));
  line("CPU", String(ESP.getCpuFreqMHz()) + " MHz");
  line("Heap", formatBytes(ESP.getFreeHeap()));
  line("Min heap", formatBytes(ESP.getMinFreeHeap()));
  line("Sketch", formatBytes(ESP.getSketchSize()));
  line("App free", formatBytes(ESP.getFreeSketchSpace()));
  line("PSRAM", formatBytes(ESP.getFreePsram()) + " / " + formatBytes(ESP.getPsramSize()));
  line("Battery", battery_.statusText());
  line("RTC", timeService_.rtcAvailable() ? "Available" : "Unavailable");
  line("WiFi", wifi_.statusText());
  line("MAC", WiFi.macAddress());

  M5.Display.setTextDatum(middle_center);
  M5.Display.fillRoundRect(138, 392, 190, 48, 16, theme.button);
  M5.Display.setTextColor(theme.foreground, theme.button);
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
