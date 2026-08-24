#include "iris/services/PowerManager.h"

#include <M5Unified.h>

#include "iris/AppConfig.h"

namespace iris {

namespace {
constexpr uint32_t kRuntimeActiveCpuMhz = 80;
constexpr uint32_t kBalancedActiveCpuMhz = 160;
constexpr uint32_t kPerformanceCpuMhz = 240;
constexpr uint32_t kRuntimeIdleCpuMhz = 80;
constexpr uint32_t kBalancedIdleCpuMhz = 80;
constexpr uint16_t kActiveLoopDelayMs = 5;
constexpr uint16_t kBalancedDimLoopDelayMs = 25;
constexpr uint16_t kRuntimeDimLoopDelayMs = 50;
constexpr uint16_t kBalancedSleepLoopDelayMs = 80;
constexpr uint16_t kRuntimeSleepLoopDelayMs = 140;
constexpr uint16_t kDimForegroundUpdateMs = 1000;
constexpr uint16_t kPerformanceForegroundUpdateMs = 100;
}  // namespace

void PowerManager::begin() {
  lastActivityMs_ = millis();
  state_ = DisplayPowerState::Active;
  applyActiveDisplay();
  applyCpuMhz(activeCpuMhz(nullptr));
}

void PowerManager::userActivity(uint32_t nowMs) {
  lastActivityMs_ = nowMs;
  if (state_ == DisplayPowerState::Sleeping) {
    wake(nowMs);
    return;
  }
  if (state_ == DisplayPowerState::Dimmed) {
    state_ = DisplayPowerState::Active;
    applyActiveDisplay();
  }
}

void PowerManager::update(uint32_t nowMs, const AppDescriptor* app) {
  const uint32_t idle = idleMs(nowMs);
  const uint16_t sleepSeconds = settings_.sleepTimeoutSeconds();

  if (sleepSeconds > 0 &&
      state_ != DisplayPowerState::Sleeping &&
      idle >= static_cast<uint32_t>(sleepSeconds) * 1000UL) {
    state_ = DisplayPowerState::Sleeping;
    applySleepDisplay();
    applyCpuMhz(idleCpuMhz(app));
    return;
  }

  if (state_ == DisplayPowerState::Active &&
      idle >= static_cast<uint32_t>(settings_.dimTimeoutSeconds()) * 1000UL) {
    state_ = DisplayPowerState::Dimmed;
    applyDimDisplay();
  }

  applyCpuMhz(state_ == DisplayPowerState::Active ? activeCpuMhz(app) : idleCpuMhz(app));
}

void PowerManager::wake(uint32_t nowMs) {
  lastActivityMs_ = nowMs;
  state_ = DisplayPowerState::Active;
  M5.Display.wakeup();
  applyActiveDisplay();
  applyCpuMhz(activeCpuMhz(nullptr));
}

const char* PowerManager::stateName() const {
  switch (state_) {
    case DisplayPowerState::Active: return "Active";
    case DisplayPowerState::Dimmed: return "Dimmed";
    case DisplayPowerState::Sleeping: return "Sleeping";
  }
  return "Unknown";
}

uint16_t PowerManager::loopDelayMs(const AppDescriptor* app) const {
  if (settings_.powerProfile() == PowerProfile::Performance || appNeedsPerformance(app)) {
    return kActiveLoopDelayMs;
  }
  if (state_ == DisplayPowerState::Sleeping) {
    return settings_.powerProfile() == PowerProfile::Runtime ? kRuntimeSleepLoopDelayMs
                                                             : kBalancedSleepLoopDelayMs;
  }
  if (state_ == DisplayPowerState::Dimmed) {
    return settings_.powerProfile() == PowerProfile::Runtime ? kRuntimeDimLoopDelayMs
                                                             : kBalancedDimLoopDelayMs;
  }
  return kActiveLoopDelayMs;
}

uint16_t PowerManager::foregroundUpdateIntervalMs(const AppDescriptor* app) const {
  if (state_ == DisplayPowerState::Sleeping) return 0;
  if (state_ == DisplayPowerState::Active) return 0;
  if (settings_.powerProfile() == PowerProfile::Performance || appNeedsPerformance(app)) {
    return kPerformanceForegroundUpdateMs;
  }
  return kDimForegroundUpdateMs;
}

bool PowerManager::appNeedsPerformance(const AppDescriptor* app) const {
  if (!app) return false;
  return app->kind == AppKind::Fidget || app->kind == AppKind::Developer;
}

bool PowerManager::profileAllowsCpuScaling() const {
  return settings_.powerProfile() != PowerProfile::Performance;
}

uint32_t PowerManager::activeCpuMhz(const AppDescriptor* app) const {
  if (!profileAllowsCpuScaling() || appNeedsPerformance(app)) return kPerformanceCpuMhz;
  if (settings_.powerProfile() == PowerProfile::Runtime) return kRuntimeActiveCpuMhz;
  return kBalancedActiveCpuMhz;
}

uint32_t PowerManager::idleCpuMhz(const AppDescriptor* app) const {
  if (!profileAllowsCpuScaling() || appNeedsPerformance(app)) return kPerformanceCpuMhz;
  if (settings_.powerProfile() == PowerProfile::Runtime) return kRuntimeIdleCpuMhz;
  return kBalancedIdleCpuMhz;
}

void PowerManager::applyCpuMhz(uint32_t mhz) {
  if (mhz == currentCpuMhz_) return;
  if (setCpuFrequencyMhz(mhz)) {
    currentCpuMhz_ = mhz;
  }
}

void PowerManager::applyActiveDisplay() {
  M5.Display.setBrightness(settings_.activeBrightness());
}

void PowerManager::applyDimDisplay() {
  M5.Display.setBrightness(settings_.dimBrightness());
}

void PowerManager::applySleepDisplay() {
  M5.Power.setVibration(0);
  M5.Display.sleep();
}

}  // namespace iris
