#pragma once

#include <Arduino.h>

namespace iris {

struct BatterySnapshot {
  int percent = -1;
  bool charging = false;
  bool chargingKnown = false;
};

class BatteryService {
 public:
  void begin();
  void update(uint32_t nowMs);

  BatterySnapshot snapshot() const { return snapshot_; }
  String statusText() const;

 private:
  void refresh();

  BatterySnapshot snapshot_;
  uint32_t lastRefreshMs_ = 0;
};

}  // namespace iris
