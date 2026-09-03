#pragma once

#include <Arduino.h>

namespace iris {

class StatusLightService {
 public:
  void begin(bool enabled);
  void update(uint32_t nowMs, bool charging, bool batteryLow);
  void setEnabled(bool enabled);
  bool enabled() const { return enabled_; }
  bool available() const { return available_; }
  const char* capabilityText() const;
  const char* statusText() const;
  void showNotification(uint8_t red, uint8_t green, uint8_t blue);
  void clearNotification();

 private:
  void apply(uint32_t nowMs);
  void setPhysicalColor(uint8_t red, uint8_t green, uint8_t blue);

  bool enabled_ = false;
  bool available_ = false;
  bool pm1GreenLedAvailable_ = false;
  bool m5UnifiedLedAvailable_ = false;
  bool charging_ = false;
  bool batteryLow_ = false;
  bool notificationActive_ = false;
  uint8_t red_ = 0;
  uint8_t green_ = 40;
  uint8_t blue_ = 0;
  bool lastPhysicalValid_ = false;
  uint8_t lastPhysicalRed_ = 255;
  uint8_t lastPhysicalGreen_ = 255;
  uint8_t lastPhysicalBlue_ = 255;
};

}  // namespace iris
