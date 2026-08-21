#pragma once

#include <Arduino.h>
#include <Preferences.h>

namespace iris {

constexpr uint8_t kWidgetBattery = 1 << 0;
constexpr uint8_t kWidgetDate = 1 << 1;
constexpr uint8_t kWidgetSeconds = 1 << 2;
constexpr uint8_t kWidgetWifi = 1 << 3;
constexpr uint8_t kWidgetComplication = 1 << 4;
constexpr uint8_t kDefaultWidgetMask = kWidgetBattery | kWidgetDate | kWidgetSeconds | kWidgetComplication;

constexpr uint8_t kComplicationNone = 0;
constexpr uint8_t kComplicationUptime = 1;
constexpr uint8_t kComplicationCount = 2;

inline const char* complicationName(uint8_t id) {
  switch (id % kComplicationCount) {
    case kComplicationUptime: return "Uptime";
    default: return "Off";
  }
}

class SettingsStore {
 public:
  void begin();

  uint8_t volume() const { return volume_; }
  void setVolume(uint8_t value);

  bool wifiEnabled() const { return wifiEnabled_; }
  void setWifiEnabled(bool enabled);

  uint8_t watchBackground() const { return watchBackground_; }
  void setWatchBackground(uint8_t value);
  uint8_t themeId() const { return watchBackground_; }
  void setThemeId(uint8_t value) { setWatchBackground(value); }

  uint8_t widgetMask() const { return widgetMask_; }
  void setWidgetMask(uint8_t value);
  bool widgetEnabled(uint8_t widget) const { return (widgetMask_ & widget) != 0; }
  void setWidgetEnabled(uint8_t widget, bool enabled);

  uint8_t complicationId() const { return complicationId_; }
  void setComplicationId(uint8_t value);

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

  bool autoRotate() const { return autoRotate_; }
  void setAutoRotate(bool enabled);

  bool indicatorLightEnabled() const { return indicatorLightEnabled_; }
  void setIndicatorLightEnabled(bool enabled);

  uint16_t touchDelayMs() const { return touchDelayMs_; }
  void setTouchDelayMs(uint16_t value);

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
  bool autoRotate_ = true;
  bool indicatorLightEnabled_ = false;
  uint16_t touchDelayMs_ = 150;
  uint8_t widgetMask_ = kDefaultWidgetMask;
  uint8_t complicationId_ = kComplicationUptime;
};

}  // namespace iris
