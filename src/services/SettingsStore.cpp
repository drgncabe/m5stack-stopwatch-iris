#include "iris/services/SettingsStore.h"
#include "iris/AppConfig.h"

namespace iris {

void SettingsStore::begin() {
  prefs_.begin("iris", false);
  volume_ = prefs_.getUChar("volume", config::kDefaultVolume);
  wifiEnabled_ = prefs_.getBool("wifi_on", true);
  watchBackground_ = prefs_.getUChar("watch_bg", 0);
}

void SettingsStore::setVolume(uint8_t value) {
  volume_ = value;
  prefs_.putUChar("volume", volume_);
}

void SettingsStore::setWifiEnabled(bool enabled) {
  wifiEnabled_ = enabled;
  prefs_.putBool("wifi_on", wifiEnabled_);
}

void SettingsStore::setWatchBackground(uint8_t value) {
  watchBackground_ = value;
  prefs_.putUChar("watch_bg", watchBackground_);
}

}  // namespace iris
