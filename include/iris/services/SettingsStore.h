#pragma once

#include <Arduino.h>
#include <Preferences.h>

namespace iris {

class SettingsStore {
 public:
  void begin();

  uint8_t volume() const { return volume_; }
  void setVolume(uint8_t value);

  bool wifiEnabled() const { return wifiEnabled_; }
  void setWifiEnabled(bool enabled);

  uint8_t watchBackground() const { return watchBackground_; }
  void setWatchBackground(uint8_t value);

  uint8_t activeBrightness() const { return activeBrightness_; }
  void setActiveBrightness(uint8_t value);

  uint16_t dimTimeoutSeconds() const { return dimTimeoutSeconds_; }
  void setDimTimeoutSeconds(uint16_t value);

  uint16_t sleepTimeoutSeconds() const { return sleepTimeoutSeconds_; }
  void setSleepTimeoutSeconds(uint16_t value);

  bool wifiOnDemand() const { return wifiOnDemand_; }
  void setWifiOnDemand(bool enabled);

  bool lowPowerFace() const { return lowPowerFace_; }
  void setLowPowerFace(bool enabled);

 private:
  Preferences prefs_;
  uint8_t volume_ = 96;
  bool wifiEnabled_ = true;
  uint8_t watchBackground_ = 0;
  uint8_t activeBrightness_ = 96;
  uint16_t dimTimeoutSeconds_ = 20;
  uint16_t sleepTimeoutSeconds_ = 90;
  bool wifiOnDemand_ = false;
  bool lowPowerFace_ = false;
};

}  // namespace iris
