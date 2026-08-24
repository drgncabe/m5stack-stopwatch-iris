#pragma once

#include <Arduino.h>
#include <Preferences.h>

namespace iris {

struct Vec3 {
  float x;
  float y;
  float z;
};

struct ImuCalibrationData {
  static constexpr uint8_t kVersion = 1;

  uint8_t version = kVersion;
  bool valid = false;
  uint32_t calibratedAtMs = 0;
  uint16_t sampleCount = 0;
  Vec3 accelOffset{0.0f, 0.0f, 0.0f};
  Vec3 gyroBias{0.0f, 0.0f, 0.0f};
  Vec3 upReference{0.0f, -1.0f, 0.0f};
  Vec3 downReference{0.0f, 1.0f, 0.0f};
  Vec3 leftReference{-1.0f, 0.0f, 0.0f};
  Vec3 rightReference{1.0f, 0.0f, 0.0f};
  Vec3 faceUpReference{0.0f, 0.0f, 1.0f};
  Vec3 faceDownReference{0.0f, 0.0f, -1.0f};
};

enum class PowerProfile : uint8_t {
  Runtime = 0,
  Balanced = 1,
  Performance = 2,
};

enum class CountryRegion : uint8_t {
  UnitedStates = 0,
  UnitedKingdom = 1,
  Europe = 2,
};

constexpr uint8_t kCountryRegionCount = static_cast<uint8_t>(CountryRegion::Europe) + 1;

enum class DateFormat : uint8_t {
  MonthDayYear = 0,
  DayMonthYear = 1,
  YearMonthDay = 2,
  MonthNameDay = 3,
  DayMonthName = 4,
};

constexpr uint8_t kDateFormatCount = static_cast<uint8_t>(DateFormat::DayMonthName) + 1;

enum class TimeFormat : uint8_t {
  TwelveHour = 0,
  TwentyFourHour = 1,
};

constexpr uint8_t kTimeFormatCount = static_cast<uint8_t>(TimeFormat::TwentyFourHour) + 1;

enum class TimeZoneId : uint8_t {
  Eastern = 0,
  Central = 1,
  Mountain = 2,
  Pacific = 3,
  Utc = 4,
  London = 5,
  CentralEurope = 6,
};

inline const char* powerProfileName(PowerProfile profile) {
  switch (profile) {
    case PowerProfile::Runtime: return "Runtime";
    case PowerProfile::Performance: return "Performance";
    default: return "Balanced";
  }
}

inline const char* countryRegionName(CountryRegion region) {
  switch (region) {
    case CountryRegion::UnitedKingdom: return "United Kingdom";
    case CountryRegion::Europe: return "Europe";
    default: return "United States";
  }
}

inline const char* countryRegionCode(CountryRegion region) {
  switch (region) {
    case CountryRegion::UnitedKingdom: return "GB";
    case CountryRegion::Europe: return "EU";
    default: return "US";
  }
}

inline const char* localeCode(CountryRegion region) {
  switch (region) {
    case CountryRegion::UnitedKingdom: return "en-GB";
    case CountryRegion::Europe: return "en-150";
    default: return "en-US";
  }
}

inline const char* dateFormatName(DateFormat format) {
  switch (format) {
    case DateFormat::DayMonthYear: return "DD/MM/YYYY";
    case DateFormat::YearMonthDay: return "YYYY-MM-DD";
    case DateFormat::MonthNameDay: return "Aug 23, 2026";
    case DateFormat::DayMonthName: return "23 Aug 2026";
    default: return "MM/DD/YYYY";
  }
}

inline const char* timeFormatName(TimeFormat format) {
  switch (format) {
    case TimeFormat::TwentyFourHour: return "24-hour";
    default: return "12-hour";
  }
}

inline const char* timeZoneName(TimeZoneId zone) {
  switch (zone) {
    case TimeZoneId::Central: return "Central";
    case TimeZoneId::Mountain: return "Mountain";
    case TimeZoneId::Pacific: return "Pacific";
    case TimeZoneId::London: return "London";
    case TimeZoneId::CentralEurope: return "Central Europe";
    case TimeZoneId::Utc: return "UTC";
    default: return "Eastern";
  }
}

inline const char* timeZoneIanaName(TimeZoneId zone) {
  switch (zone) {
    case TimeZoneId::Central: return "America/Chicago";
    case TimeZoneId::Mountain: return "America/Denver";
    case TimeZoneId::Pacific: return "America/Los_Angeles";
    case TimeZoneId::London: return "Europe/London";
    case TimeZoneId::CentralEurope: return "Europe/Berlin";
    case TimeZoneId::Utc: return "Etc/UTC";
    default: return "America/New_York";
  }
}

inline const char* timeZonePosix(TimeZoneId zone) {
  switch (zone) {
    case TimeZoneId::Central: return "CST6CDT,M3.2.0,M11.1.0";
    case TimeZoneId::Mountain: return "MST7MDT,M3.2.0,M11.1.0";
    case TimeZoneId::Pacific: return "PST8PDT,M3.2.0,M11.1.0";
    case TimeZoneId::London: return "GMT0BST,M3.5.0/1,M10.5.0";
    case TimeZoneId::CentralEurope: return "CET-1CEST,M3.5.0,M10.5.0/3";
    case TimeZoneId::Utc: return "UTC0";
    default: return "EST5EDT,M3.2.0,M11.1.0";
  }
}

constexpr uint8_t kTimeZoneCount = static_cast<uint8_t>(TimeZoneId::CentralEurope) + 1;

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

  uint8_t dimBrightness() const { return dimBrightness_; }
  void setDimBrightness(uint8_t value);

  uint16_t dimTimeoutSeconds() const { return dimTimeoutSeconds_; }
  void setDimTimeoutSeconds(uint16_t value);

  uint16_t sleepTimeoutSeconds() const { return sleepTimeoutSeconds_; }
  void setSleepTimeoutSeconds(uint16_t value);

  bool wifiOnDemand() const { return wifiOnDemand_; }
  void setWifiOnDemand(bool enabled);

  bool lowPowerFace() const { return lowPowerFace_; }
  void setLowPowerFace(bool enabled);

  PowerProfile powerProfile() const { return powerProfile_; }
  void setPowerProfile(PowerProfile profile);

  CountryRegion countryRegion() const { return countryRegion_; }
  void setCountryRegion(CountryRegion region);

  DateFormat dateFormat() const { return dateFormat_; }
  void setDateFormat(DateFormat format);

  TimeFormat timeFormat() const { return timeFormat_; }
  void setTimeFormat(TimeFormat format);

  TimeZoneId timeZone() const { return timeZone_; }
  void setTimeZone(TimeZoneId zone);

  bool automaticTimeEnabled() const { return automaticTimeEnabled_; }
  void setAutomaticTimeEnabled(bool enabled);

  bool autoRotate() const { return autoRotate_; }
  void setAutoRotate(bool enabled);

  bool indicatorLightEnabled() const { return indicatorLightEnabled_; }
  void setIndicatorLightEnabled(bool enabled);

  uint16_t touchDelayMs() const { return touchDelayMs_; }
  void setTouchDelayMs(uint16_t value);

  const ImuCalibrationData& imuCalibration() const { return imuCalibration_; }
  bool imuCalibrationValid() const { return imuCalibration_.valid; }
  void saveImuCalibration(const ImuCalibrationData& data);
  void clearImuCalibration();
  float accelOffsetX() const { return imuCalibration_.accelOffset.x; }
  float accelOffsetY() const { return imuCalibration_.accelOffset.y; }
  float accelOffsetZ() const { return imuCalibration_.accelOffset.z; }
  void setAccelCalibration(float offsetX, float offsetY, float offsetZ);
  void resetAccelCalibration();

 private:
  Preferences prefs_;
  uint8_t volume_ = 96;
  bool wifiEnabled_ = true;
  uint8_t watchBackground_ = 0;
  uint8_t activeBrightness_ = 96;
  uint8_t dimBrightness_ = 18;
  uint16_t dimTimeoutSeconds_ = 20;
  uint16_t sleepTimeoutSeconds_ = 90;
  bool wifiOnDemand_ = false;
  bool lowPowerFace_ = false;
  PowerProfile powerProfile_ = PowerProfile::Balanced;
  CountryRegion countryRegion_ = CountryRegion::UnitedStates;
  DateFormat dateFormat_ = DateFormat::MonthDayYear;
  TimeFormat timeFormat_ = TimeFormat::TwelveHour;
  TimeZoneId timeZone_ = TimeZoneId::Eastern;
  bool automaticTimeEnabled_ = true;
  bool autoRotate_ = true;
  bool indicatorLightEnabled_ = false;
  uint16_t touchDelayMs_ = 150;
  uint8_t widgetMask_ = kDefaultWidgetMask;
  uint8_t complicationId_ = kComplicationUptime;
  ImuCalibrationData imuCalibration_;
};

}  // namespace iris
