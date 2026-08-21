#include "iris/services/StatusLightService.h"

#include <M5Unified.h>

namespace iris {

void StatusLightService::begin(bool enabled) {
  enabled_ = enabled;
  if (M5.Led.isEnabled()) {
    M5.Led.setBrightness(64);
  }
  apply();
}

void StatusLightService::setEnabled(bool enabled) {
  enabled_ = enabled;
  apply();
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
  if (!M5.Led.isEnabled()) return;

  if (!enabled_) {
    M5.Led.setAllColor(0, 0, 0);
    M5.Led.display();
    return;
  }

  if (notificationActive_) {
    M5.Led.setAllColor(red_, green_, blue_);
  } else {
    M5.Led.setAllColor(0, 40, 0);
  }
  M5.Led.display();
}

}  // namespace iris
