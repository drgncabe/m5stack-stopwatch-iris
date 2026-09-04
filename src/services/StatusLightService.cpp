#include "iris/services/StatusLightService.h"

#include <M5Unified.h>
#include <M5PM1.h>

namespace iris {

namespace {
M5PM1 sPm1;
bool sPm1Initialized = false;
constexpr uint32_t kChargingBreathMs = 1800;
constexpr uint32_t kLowBatteryBlinkMs = 700;

bool initPm1() {
  if (sPm1Initialized) return true;
  const m5pm1_err_t err = sPm1.begin(&M5.In_I2C, M5PM1_DEFAULT_ADDR, M5PM1_I2C_FREQ_100K);
  sPm1Initialized = err == M5PM1_OK;
  Serial.printf("[StatusLight] M5PM1 init: %s (%d)\n",
                sPm1Initialized ? "ok" : "failed", static_cast<int>(err));
  return sPm1Initialized;
}
}  // namespace

void StatusLightService::begin(bool enabled) {
  pm1GreenLedAvailable_ = M5.getBoard() == m5::board_t::board_M5StopWatch && initPm1();
  m5UnifiedLedAvailable_ = !pm1GreenLedAvailable_ && M5.Led.isEnabled();
  available_ = pm1GreenLedAvailable_ || m5UnifiedLedAvailable_;
  enabled_ = enabled;
  Serial.printf("[StatusLight] PM1 green LED available: %s\n",
                pm1GreenLedAvailable_ ? "yes" : "no");
  Serial.printf("[StatusLight] M5Unified LED fallback available: %s\n",
                m5UnifiedLedAvailable_ ? "yes" : "no");
  Serial.println("[StatusLight] StopWatch USB-C LED is single-color green via M5PM1 LED_EN");
  if (m5UnifiedLedAvailable_) {
    M5.Led.setBrightness(64);
  }
  apply(millis());
}

void StatusLightService::update(uint32_t nowMs, bool charging, bool batteryLow) {
  charging_ = charging;
  batteryLow_ = batteryLow;
  apply(nowMs);
}

void StatusLightService::setEnabled(bool enabled) {
  enabled_ = enabled;
  Serial.printf("[StatusLight] Logical LED requested: %s\n", enabled_ ? "on" : "off");
  apply(millis());
}

const char* StatusLightService::capabilityText() const {
  if (pm1GreenLedAvailable_) return "PM1 green LED";
  if (m5UnifiedLedAvailable_) return "M5Unified LED";
  return "No user LED";
}

const char* StatusLightService::statusText() const {
  if (!available_) return "Unavailable";
  if (!enabled_) return "Off";
  if (notificationActive_) return "On notification";
  if (batteryLow_) return "On battery low";
  if (charging_) return "On charging";
  return "On ready";
}

void StatusLightService::showNotification(uint8_t red, uint8_t green, uint8_t blue) {
  notificationActive_ = true;
  red_ = red;
  green_ = green;
  blue_ = blue;
  apply(millis());
}

void StatusLightService::clearNotification() {
  notificationActive_ = false;
  red_ = 0;
  green_ = 40;
  blue_ = 0;
  apply(millis());
}

void StatusLightService::apply(uint32_t nowMs) {
  if (!available_) return;

  if (!enabled_) {
    setPhysicalColor(0, 0, 0);
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

  if (!notificationActive_ && batteryLow_) {
    const bool on = ((nowMs / (kLowBatteryBlinkMs / 2)) % 2) == 0;
    red = on ? 48 : 0;
    green = on ? 48 : 0;
    blue = 0;
  } else if (!notificationActive_ && charging_) {
    const uint32_t phase = nowMs % kChargingBreathMs;
    const uint32_t half = kChargingBreathMs / 2;
    red = 0;
    if (pm1GreenLedAvailable_) {
      green = phase < half ? 48 : 0;
    } else {
      const uint32_t ramp = phase < half ? phase : kChargingBreathMs - phase;
      green = static_cast<uint8_t>(8 + ((ramp * 56) / half));
    }
    blue = 0;
  } else if (!notificationActive_) {
    red = 0;
    green = 0;
    blue = 0;
  }

  setPhysicalColor(red, green, blue);
}

void StatusLightService::setPhysicalColor(uint8_t red, uint8_t green, uint8_t blue) {
  if (lastPhysicalValid_ && red == lastPhysicalRed_ && green == lastPhysicalGreen_ &&
      blue == lastPhysicalBlue_) {
    return;
  }

  if (pm1GreenLedAvailable_) {
    const bool on = red > 0 || green > 0 || blue > 0;
    const bool wasOn = lastPhysicalRed_ > 0 || lastPhysicalGreen_ > 0 || lastPhysicalBlue_ > 0;
    if (lastPhysicalValid_ && on == wasOn) return;

    const m5pm1_err_t err = sPm1.setLedEnLevel(on);
    Serial.printf("[StatusLight] PM1 green LED %s from RGB(%u,%u,%u): %s (%d)\n",
                  on ? "on" : "off", red, green, blue,
                  err == M5PM1_OK ? "ok" : "failed", static_cast<int>(err));
  } else {
    M5.Led.setAllColor(red, green, blue);
    M5.Led.display();
  }

  lastPhysicalValid_ = true;
  lastPhysicalRed_ = red;
  lastPhysicalGreen_ = green;
  lastPhysicalBlue_ = blue;
}

}  // namespace iris
