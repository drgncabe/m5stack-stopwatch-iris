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
  autoRotate_ = prefs_.getBool("auto_rotate", true);
  indicatorLightEnabled_ = prefs_.getBool("light_on", false);
  touchDelayMs_ = prefs_.getUShort("touch_ms", 150);
  widgetMask_ = prefs_.getUChar("widgets", kDefaultWidgetMask);
  complicationId_ = prefs_.getUChar("comp_id", kComplicationUptime) % kComplicationCount;
  accelOffsetX_ = prefs_.getFloat("accel_x", 0.0f);
  accelOffsetY_ = prefs_.getFloat("accel_y", 0.0f);
  accelOffsetZ_ = prefs_.getFloat("accel_z", 0.0f);
  if ((widgetMask_ & kWidgetComplication) == 0 || complicationId_ == kComplicationNone) {
    complicationId_ = kComplicationNone;
    widgetMask_ &= ~kWidgetComplication;
  }
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

void SettingsStore::setWidgetMask(uint8_t value) {
  widgetMask_ = value;
  prefs_.putUChar("widgets", widgetMask_);
}

void SettingsStore::setWidgetEnabled(uint8_t widget, bool enabled) {
  if (enabled) {
    setWidgetMask(widgetMask_ | widget);
  } else {
    setWidgetMask(widgetMask_ & ~widget);
  }
}

void SettingsStore::setComplicationId(uint8_t value) {
  complicationId_ = value % kComplicationCount;
  prefs_.putUChar("comp_id", complicationId_);
  setWidgetEnabled(kWidgetComplication, complicationId_ != kComplicationNone);
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

void SettingsStore::setAutoRotate(bool enabled) {
  autoRotate_ = enabled;
  prefs_.putBool("auto_rotate", autoRotate_);
}

void SettingsStore::setIndicatorLightEnabled(bool enabled) {
  indicatorLightEnabled_ = enabled;
  prefs_.putBool("light_on", indicatorLightEnabled_);
}

void SettingsStore::setTouchDelayMs(uint16_t value) {
  touchDelayMs_ = value;
  prefs_.putUShort("touch_ms", touchDelayMs_);
}

void SettingsStore::setAccelCalibration(float offsetX, float offsetY, float offsetZ) {
  accelOffsetX_ = offsetX;
  accelOffsetY_ = offsetY;
  accelOffsetZ_ = offsetZ;
  prefs_.putFloat("accel_x", accelOffsetX_);
  prefs_.putFloat("accel_y", accelOffsetY_);
  prefs_.putFloat("accel_z", accelOffsetZ_);
}

void SettingsStore::resetAccelCalibration() {
  setAccelCalibration(0.0f, 0.0f, 0.0f);
}

}  // namespace iris
