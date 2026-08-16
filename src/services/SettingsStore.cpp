#include "iris/services/SettingsStore.h"
#include "iris/AppConfig.h"

namespace iris {

void SettingsStore::begin() {
  prefs_.begin("iris", false);
  volume_ = prefs_.getUChar("volume", config::kDefaultVolume);
  wifiEnabled_ = prefs_.getBool("wifi_on", true);
}

void SettingsStore::setVolume(uint8_t value) {
  volume_ = value;
  prefs_.putUChar("volume", volume_);
}

void SettingsStore::setWifiEnabled(bool enabled) {
  wifiEnabled_ = enabled;
  prefs_.putBool("wifi_on", wifiEnabled_);
}

}  // namespace iris
