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

 private:
  Preferences prefs_;
  uint8_t volume_ = 96;
  bool wifiEnabled_ = true;
  uint8_t watchBackground_ = 0;
};

}  // namespace iris
