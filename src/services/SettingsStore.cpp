#include "iris/services/SettingsStore.h"
#include "iris/AppConfig.h"

namespace iris {

void SettingsStore::begin() {
  prefs_.begin("iris", false);
  volume_ = prefs_.getUChar("volume", config::kDefaultVolume);
  wifiEnabled_ = prefs_.getBool("wifi_on", true);
  watchBackground_ = prefs_.getUChar("watch_bg", 0);
  activeBrightness_ = prefs_.getUChar("bright", config::kActiveBrightness);
  dimTimeoutSeconds_ = prefs_.getUShort("dim_sec", config::kDisplayDimMs / 1000UL);
  sleepTimeoutSeconds_ = prefs_.getUShort("sleep_sec", config::kDisplaySleepMs / 1000UL);
  wifiOnDemand_ = prefs_.getBool("wifi_demand", false);
  lowPowerFace_ = prefs_.getBool("low_face", false);
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

void SettingsStore::setActiveBrightness(uint8_t value) {
  activeBrightness_ = value;
  prefs_.putUChar("bright", activeBrightness_);
}

void SettingsStore::setDimTimeoutSeconds(uint16_t value) {
  dimTimeoutSeconds_ = value;
  prefs_.putUShort("dim_sec", dimTimeoutSeconds_);
}

void SettingsStore::setSleepTimeoutSeconds(uint16_t value) {
  sleepTimeoutSeconds_ = value;
  prefs_.putUShort("sleep_sec", sleepTimeoutSeconds_);
}

void SettingsStore::setWifiOnDemand(bool enabled) {
  wifiOnDemand_ = enabled;
  prefs_.putBool("wifi_demand", wifiOnDemand_);
}

void SettingsStore::setLowPowerFace(bool enabled) {
  lowPowerFace_ = enabled;
  prefs_.putBool("low_face", lowPowerFace_);
}

}  // namespace iris
