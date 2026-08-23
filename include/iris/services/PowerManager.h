#pragma once

#include <Arduino.h>

#include "iris/core/AppManager.h"
#include "iris/screens/Screen.h"
#include "iris/services/SettingsStore.h"

namespace iris {

enum class DisplayPowerState : uint8_t {
  Active,
  Dimmed,
  Sleeping,
};

class PowerManager {
 public:
  explicit PowerManager(SettingsStore& settings) : settings_(settings) {}

  void begin();
  void userActivity(uint32_t nowMs);
  void update(uint32_t nowMs, const AppDescriptor* app);
  void wake(uint32_t nowMs);

  DisplayPowerState state() const { return state_; }
  const char* stateName() const;
  uint32_t idleMs(uint32_t nowMs) const { return nowMs - lastActivityMs_; }
  uint32_t currentCpuMhz() const { return currentCpuMhz_; }
  const char* profileName() const { return powerProfileName(settings_.powerProfile()); }

 private:
  bool appNeedsPerformance(const AppDescriptor* app) const;
  bool profileAllowsCpuScaling() const;
  uint32_t activeCpuMhz(const AppDescriptor* app) const;
  uint32_t idleCpuMhz(const AppDescriptor* app) const;
  void applyCpuMhz(uint32_t mhz);
  void applyActiveDisplay();
  void applyDimDisplay();
  void applySleepDisplay();

  SettingsStore& settings_;
  uint32_t lastActivityMs_ = 0;
  uint32_t currentCpuMhz_ = 0;
  DisplayPowerState state_ = DisplayPowerState::Active;
};

}  // namespace iris
