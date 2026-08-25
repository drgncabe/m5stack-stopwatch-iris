#include "iris/services/StatusLightService.h"

#include <M5Unified.h>
#include <M5PM1.h>

namespace iris {

namespace {
M5PM1 sPm1;
bool sPm1Initialized = false;

bool initPm1() {
  if (sPm1Initialized) return true;
  const m5pm1_err_t err = sPm1.begin(&M5.In_I2C, M5PM1_DEFAULT_ADDR, M5PM1_I2C_FREQ_100K);
  sPm1Initialized = err == M5PM1_OK;
  Serial.printf("[StatusLight] M5PM1 NeoPixel init: %s (%d)\n",
                sPm1Initialized ? "ok" : "failed", static_cast<int>(err));
  return sPm1Initialized;
}
}  // namespace

void StatusLightService::begin(bool enabled) {
  pm1NeoPixelAvailable_ = M5.getBoard() == m5::board_t::board_M5StopWatch && initPm1();
  m5UnifiedLedAvailable_ = !pm1NeoPixelAvailable_ && M5.Led.isEnabled();
  available_ = pm1NeoPixelAvailable_ || m5UnifiedLedAvailable_;
  enabled_ = enabled;
  Serial.printf("[StatusLight] PM1 NeoPixel available: %s\n",
                pm1NeoPixelAvailable_ ? "yes" : "no");
  Serial.printf("[StatusLight] M5Unified LED fallback available: %s\n",
                m5UnifiedLedAvailable_ ? "yes" : "no");
  Serial.println("[StatusLight] StopWatch IO map lists M5PM1 GPIO0 as WAKE/IRQ/NEOPIXEL");
  if (m5UnifiedLedAvailable_) {
    M5.Led.setBrightness(64);
  }
  apply();
}

void StatusLightService::setEnabled(bool enabled) {
  enabled_ = enabled;
  Serial.printf("[StatusLight] Logical LED requested: %s\n", enabled_ ? "on" : "off");
  apply();
}

const char* StatusLightService::capabilityText() const {
  if (pm1NeoPixelAvailable_) return "PM1 NeoPixel";
  if (m5UnifiedLedAvailable_) return "M5Unified LED";
  return "No user LED";
}

const char* StatusLightService::statusText() const {
  if (!available_) return "Unavailable";
  return enabled_ ? "On" : "Off";
}

void StatusLightService::showNotification(uint8_t red, uint8_t green, uint8_t blue) {
  notificationActive_ = true;
  red_ = red;
  green_ = green;
  blue_ = blue;
  apply();
}

void StatusLightService::clearNotification() {
  notificationActive_ = false;
  red_ = 0;
  green_ = 40;
  blue_ = 0;
  apply();
}

void StatusLightService::apply() {
  if (!available_) return;

  if (!enabled_) {
    if (pm1NeoPixelAvailable_) {
      const m5pm1_err_t err = sPm1.disableLeds();
      Serial.printf("[StatusLight] PM1 NeoPixel off: %s (%d)\n",
                    err == M5PM1_OK ? "ok" : "failed", static_cast<int>(err));
    } else {
      M5.Led.setAllColor(0, 0, 0);
      M5.Led.display();
    }
    return;
  }

  uint8_t red = 0;
  uint8_t green = 40;
  uint8_t blue = 0;
  if (notificationActive_) {
    red = red_;
    green = green_;
    blue = blue_;
  }

  if (pm1NeoPixelAvailable_) {
    const m5pm1_rgb_t color{red, green, blue};
    const m5pm1_err_t err = sPm1.setLeds(&color, 1, 1, true);
    Serial.printf("[StatusLight] PM1 NeoPixel RGB(%u,%u,%u): %s (%d)\n",
                  red, green, blue, err == M5PM1_OK ? "ok" : "failed",
                  static_cast<int>(err));
    return;
  }

  M5.Led.setAllColor(red, green, blue);
  M5.Led.display();
}

}  // namespace iris
