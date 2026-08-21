#pragma once

#include <Arduino.h>

namespace iris {

class StatusLightService {
 public:
  void begin(bool enabled);
  void setEnabled(bool enabled);
  bool enabled() const { return enabled_; }
  void showNotification(uint8_t red, uint8_t green, uint8_t blue);
  void clearNotification();

 private:
  void apply();

  bool enabled_ = false;
  bool notificationActive_ = false;
  uint8_t red_ = 0;
  uint8_t green_ = 40;
  uint8_t blue_ = 0;
};

}  // namespace iris
