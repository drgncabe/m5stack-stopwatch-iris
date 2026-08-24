#include "iris/services/SettingsStore.h"
#include "iris/AppConfig.h"

namespace iris {

void SettingsStore::begin() {
  prefs_.begin("iris", false);
  volume_ = prefs_.getUChar("volume", config::kDefaultVolume);
  wifiEnabled_ = prefs_.getBool("wifi_on", true);
  watchBackground_ = prefs_.getUChar("watch_bg", 0);
  activeBrightness_ = prefs_.getUChar("bright", config::kActiveBrightness);
  dimBrightness_ = prefs_.getUChar("dim_bright", config::kDimBrightness);
  dimTimeoutSeconds_ = prefs_.getUShort("dim_sec", config::kDisplayDimMs / 1000UL);
  sleepTimeoutSeconds_ = prefs_.getUShort("sleep_sec", config::kDisplaySleepMs / 1000UL);
  wifiOnDemand_ = prefs_.getBool("wifi_demand", false);
  lowPowerFace_ = prefs_.getBool("low_face", false);
  powerProfile_ = static_cast<PowerProfile>(prefs_.getUChar("pwr_profile", static_cast<uint8_t>(PowerProfile::Balanced)));
  if (static_cast<uint8_t>(powerProfile_) > static_cast<uint8_t>(PowerProfile::Performance)) {
    powerProfile_ = PowerProfile::Balanced;
  }
  countryRegion_ = static_cast<CountryRegion>(
      prefs_.getUChar("country", static_cast<uint8_t>(CountryRegion::UnitedStates)));
  if (static_cast<uint8_t>(countryRegion_) >= kCountryRegionCount) {
    countryRegion_ = CountryRegion::UnitedStates;
  }
  dateFormat_ = static_cast<DateFormat>(
      prefs_.getUChar("date_fmt", static_cast<uint8_t>(DateFormat::MonthDayYear)));
  if (static_cast<uint8_t>(dateFormat_) >= kDateFormatCount) {
    dateFormat_ = DateFormat::MonthDayYear;
  }
  timeFormat_ = static_cast<TimeFormat>(
      prefs_.getUChar("time_fmt", static_cast<uint8_t>(TimeFormat::TwelveHour)));
  if (static_cast<uint8_t>(timeFormat_) >= kTimeFormatCount) {
    timeFormat_ = TimeFormat::TwelveHour;
  }
  timeZone_ = static_cast<TimeZoneId>(
      prefs_.getUChar("tz_id", static_cast<uint8_t>(TimeZoneId::Eastern)));
  if (static_cast<uint8_t>(timeZone_) >= kTimeZoneCount) {
    timeZone_ = TimeZoneId::Eastern;
  }
  automaticTimeEnabled_ = prefs_.getBool("auto_time", true);
  lastNtpSyncEpoch_ = prefs_.getULong("ntp_epoch", 0);
  autoRotate_ = prefs_.getBool("auto_rotate", true);
  indicatorLightEnabled_ = prefs_.getBool("light_on", false);
  touchDelayMs_ = prefs_.getUShort("touch_ms", 150);
  widgetMask_ = prefs_.getUChar("widgets", kDefaultWidgetMask);
  complicationId_ = prefs_.getUChar("comp_id", kComplicationUptime) % kComplicationCount;
  imuCalibration_.version = prefs_.getUChar("imu_ver", 0);
  imuCalibration_.valid = prefs_.getBool("imu_valid", false) &&
                          imuCalibration_.version == ImuCalibrationData::kVersion;
  imuCalibration_.calibratedAtMs = prefs_.getULong("imu_when", 0);
  imuCalibration_.sampleCount = prefs_.getUShort("imu_samples", 0);
  imuCalibration_.accelOffset.x = prefs_.getFloat("accel_x", 0.0f);
  imuCalibration_.accelOffset.y = prefs_.getFloat("accel_y", 0.0f);
  imuCalibration_.accelOffset.z = prefs_.getFloat("accel_z", 0.0f);
  imuCalibration_.gyroBias.x = prefs_.getFloat("gyro_x", 0.0f);
  imuCalibration_.gyroBias.y = prefs_.getFloat("gyro_y", 0.0f);
  imuCalibration_.gyroBias.z = prefs_.getFloat("gyro_z", 0.0f);
  imuCalibration_.upReference.x = prefs_.getFloat("ref_up_x", 0.0f);
  imuCalibration_.upReference.y = prefs_.getFloat("ref_up_y", -1.0f);
  imuCalibration_.upReference.z = prefs_.getFloat("ref_up_z", 0.0f);
  imuCalibration_.downReference.x = prefs_.getFloat("ref_dn_x", 0.0f);
  imuCalibration_.downReference.y = prefs_.getFloat("ref_dn_y", 1.0f);
  imuCalibration_.downReference.z = prefs_.getFloat("ref_dn_z", 0.0f);
  imuCalibration_.leftReference.x = prefs_.getFloat("ref_lt_x", -1.0f);
  imuCalibration_.leftReference.y = prefs_.getFloat("ref_lt_y", 0.0f);
  imuCalibration_.leftReference.z = prefs_.getFloat("ref_lt_z", 0.0f);
  imuCalibration_.rightReference.x = prefs_.getFloat("ref_rt_x", 1.0f);
  imuCalibration_.rightReference.y = prefs_.getFloat("ref_rt_y", 0.0f);
  imuCalibration_.rightReference.z = prefs_.getFloat("ref_rt_z", 0.0f);
  imuCalibration_.faceUpReference.x = prefs_.getFloat("ref_fu_x", 0.0f);
  imuCalibration_.faceUpReference.y = prefs_.getFloat("ref_fu_y", 0.0f);
  imuCalibration_.faceUpReference.z = prefs_.getFloat("ref_fu_z", 1.0f);
  imuCalibration_.faceDownReference.x = prefs_.getFloat("ref_fd_x", 0.0f);
  imuCalibration_.faceDownReference.y = prefs_.getFloat("ref_fd_y", 0.0f);
  imuCalibration_.faceDownReference.z = prefs_.getFloat("ref_fd_z", -1.0f);
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

void SettingsStore::setDimBrightness(uint8_t value) {
  dimBrightness_ = value;
  prefs_.putUChar("dim_bright", dimBrightness_);
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

void SettingsStore::setPowerProfile(PowerProfile profile) {
  powerProfile_ = profile;
  prefs_.putUChar("pwr_profile", static_cast<uint8_t>(powerProfile_));
}

void SettingsStore::setCountryRegion(CountryRegion region) {
  countryRegion_ = region;
  prefs_.putUChar("country", static_cast<uint8_t>(countryRegion_));

  if (region == CountryRegion::UnitedStates) {
    setDateFormat(DateFormat::MonthDayYear);
    setTimeFormat(TimeFormat::TwelveHour);
  } else if (region == CountryRegion::UnitedKingdom) {
    setDateFormat(DateFormat::DayMonthYear);
    setTimeFormat(TimeFormat::TwentyFourHour);
  } else {
    setDateFormat(DateFormat::YearMonthDay);
    setTimeFormat(TimeFormat::TwentyFourHour);
  }
}

void SettingsStore::setDateFormat(DateFormat format) {
  dateFormat_ = format;
  prefs_.putUChar("date_fmt", static_cast<uint8_t>(dateFormat_));
}

void SettingsStore::setTimeFormat(TimeFormat format) {
  timeFormat_ = format;
  prefs_.putUChar("time_fmt", static_cast<uint8_t>(timeFormat_));
}

void SettingsStore::setTimeZone(TimeZoneId zone) {
  timeZone_ = zone;
  prefs_.putUChar("tz_id", static_cast<uint8_t>(timeZone_));
}

void SettingsStore::setAutomaticTimeEnabled(bool enabled) {
  automaticTimeEnabled_ = enabled;
  prefs_.putBool("auto_time", automaticTimeEnabled_);
}

void SettingsStore::setLastNtpSyncEpoch(uint32_t epoch) {
  lastNtpSyncEpoch_ = epoch;
  prefs_.putULong("ntp_epoch", lastNtpSyncEpoch_);
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

void SettingsStore::saveImuCalibration(const ImuCalibrationData& data) {
  imuCalibration_ = data;
  imuCalibration_.version = ImuCalibrationData::kVersion;
  imuCalibration_.valid = true;
  prefs_.putUChar("imu_ver", imuCalibration_.version);
  prefs_.putBool("imu_valid", imuCalibration_.valid);
  prefs_.putULong("imu_when", imuCalibration_.calibratedAtMs);
  prefs_.putUShort("imu_samples", imuCalibration_.sampleCount);
  prefs_.putFloat("accel_x", imuCalibration_.accelOffset.x);
  prefs_.putFloat("accel_y", imuCalibration_.accelOffset.y);
  prefs_.putFloat("accel_z", imuCalibration_.accelOffset.z);
  prefs_.putFloat("gyro_x", imuCalibration_.gyroBias.x);
  prefs_.putFloat("gyro_y", imuCalibration_.gyroBias.y);
  prefs_.putFloat("gyro_z", imuCalibration_.gyroBias.z);
  prefs_.putFloat("ref_up_x", imuCalibration_.upReference.x);
  prefs_.putFloat("ref_up_y", imuCalibration_.upReference.y);
  prefs_.putFloat("ref_up_z", imuCalibration_.upReference.z);
  prefs_.putFloat("ref_dn_x", imuCalibration_.downReference.x);
  prefs_.putFloat("ref_dn_y", imuCalibration_.downReference.y);
  prefs_.putFloat("ref_dn_z", imuCalibration_.downReference.z);
  prefs_.putFloat("ref_lt_x", imuCalibration_.leftReference.x);
  prefs_.putFloat("ref_lt_y", imuCalibration_.leftReference.y);
  prefs_.putFloat("ref_lt_z", imuCalibration_.leftReference.z);
  prefs_.putFloat("ref_rt_x", imuCalibration_.rightReference.x);
  prefs_.putFloat("ref_rt_y", imuCalibration_.rightReference.y);
  prefs_.putFloat("ref_rt_z", imuCalibration_.rightReference.z);
  prefs_.putFloat("ref_fu_x", imuCalibration_.faceUpReference.x);
  prefs_.putFloat("ref_fu_y", imuCalibration_.faceUpReference.y);
  prefs_.putFloat("ref_fu_z", imuCalibration_.faceUpReference.z);
  prefs_.putFloat("ref_fd_x", imuCalibration_.faceDownReference.x);
  prefs_.putFloat("ref_fd_y", imuCalibration_.faceDownReference.y);
  prefs_.putFloat("ref_fd_z", imuCalibration_.faceDownReference.z);
}

void SettingsStore::clearImuCalibration() {
  ImuCalibrationData data;
  imuCalibration_ = data;
  prefs_.putUChar("imu_ver", ImuCalibrationData::kVersion);
  prefs_.putBool("imu_valid", false);
  prefs_.putULong("imu_when", 0);
  prefs_.putUShort("imu_samples", 0);
  prefs_.putFloat("accel_x", 0.0f);
  prefs_.putFloat("accel_y", 0.0f);
  prefs_.putFloat("accel_z", 0.0f);
  prefs_.putFloat("gyro_x", 0.0f);
  prefs_.putFloat("gyro_y", 0.0f);
  prefs_.putFloat("gyro_z", 0.0f);
}

void SettingsStore::setAccelCalibration(float offsetX, float offsetY, float offsetZ) {
  ImuCalibrationData data = imuCalibration_;
  data.valid = true;
  data.calibratedAtMs = millis();
  data.sampleCount = 1;
  data.accelOffset = {offsetX, offsetY, offsetZ};
  saveImuCalibration(data);
}

void SettingsStore::resetAccelCalibration() {
  clearImuCalibration();
}

}  // namespace iris
