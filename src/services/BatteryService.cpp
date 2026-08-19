#include "iris/services/BatteryService.h"

#include <M5Unified.h>

namespace iris {

namespace {
constexpr uint32_t kBatteryRefreshMs = 30000;
}

void BatteryService::begin() {
  refresh();
}

void BatteryService::update(uint32_t nowMs) {
  if (lastRefreshMs_ == 0 || nowMs - lastRefreshMs_ >= kBatteryRefreshMs) {
    refresh();
    lastRefreshMs_ = nowMs;
  }
}

String BatteryService::statusText() const {
  if (snapshot_.percent < 0) return "Unknown";

  String text = String(snapshot_.percent) + "%";
  if (snapshot_.chargingKnown && snapshot_.charging) {
    text += " charging";
  }
  return text;
}

void BatteryService::refresh() {
  snapshot_.percent = static_cast<int>(M5.Power.getBatteryLevel());

  const auto charging = M5.Power.isCharging();
  snapshot_.chargingKnown = charging != m5::Power_Class::charge_unknown;
  snapshot_.charging = charging == m5::Power_Class::is_charging;
}

}  // namespace iris
