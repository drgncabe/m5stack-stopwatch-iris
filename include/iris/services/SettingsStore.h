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

 private:
  Preferences prefs_;
  uint8_t volume_ = 96;
  bool wifiEnabled_ = true;
};

}  // namespace iris
