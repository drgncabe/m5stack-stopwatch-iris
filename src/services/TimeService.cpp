#include "iris/services/TimeService.h"

#include <M5Unified.h>
#include <time.h>

#include "iris/AppConfig.h"

namespace iris {

void TimeService::begin() {
  rtcAvailable_ = M5.Rtc.isEnabled();
  Serial.printf("Iris RTC: %s\n", rtcAvailable_ ? "available" : "not available");
}

void TimeService::update(uint32_t nowMs, bool wifiConnected) {
  if (!wifiConnected) return;

  const bool resyncDue = ntpSynchronized_ &&
      (nowMs - lastNtpSyncMs_ >= config::kNtpResyncMs);

  if (!ntpRequested_ || resyncDue ||
      (!ntpSynchronized_ && nowMs - lastNtpRequestMs_ >= config::kNtpRetryMs)) {
    requestNtp(nowMs);
  }

  if (ntpRequested_ && copySystemTimeToRtc()) {
    ntpSynchronized_ = true;
    lastNtpSyncMs_ = nowMs;
  }
}

DateTimeSnapshot TimeService::now() const {
  DateTimeSnapshot snapshot;

  time_t systemNow = time(nullptr);
  if (systemNow > 1700000000) {
    struct tm localTime {};
    localtime_r(&systemNow, &localTime);
    snapshot.year = localTime.tm_year + 1900;
    snapshot.month = localTime.tm_mon + 1;
    snapshot.day = localTime.tm_mday;
    snapshot.hour = localTime.tm_hour;
    snapshot.minute = localTime.tm_min;
    snapshot.second = localTime.tm_sec;
    snapshot.weekDay = localTime.tm_wday;
    snapshot.valid = true;
    return snapshot;
  }

  if (rtcAvailable_) {
    const auto dt = M5.Rtc.getDateTime();
    snapshot.year = dt.date.year;
    snapshot.month = dt.date.month;
    snapshot.day = dt.date.date;
    snapshot.hour = dt.time.hours;
    snapshot.minute = dt.time.minutes;
    snapshot.second = dt.time.seconds;
    snapshot.weekDay = dt.date.weekDay;
    snapshot.valid = snapshot.year >= 2024;
  }

  return snapshot;
}

void TimeService::requestNtp(uint32_t nowMs) {
  configTzTime(config::kTimezone,
               config::kNtpServer1,
               config::kNtpServer2,
               config::kNtpServer3);
  ntpRequested_ = true;
  lastNtpRequestMs_ = nowMs;
  Serial.println("Iris time: NTP synchronization requested");
}

bool TimeService::copySystemTimeToRtc() {
  struct tm localTime {};
  if (!getLocalTime(&localTime, 10)) return false;
  if (localTime.tm_year + 1900 < 2024) return false;

  if (rtcAvailable_) {
    M5.Rtc.setDateTime({
        {static_cast<int16_t>(localTime.tm_year + 1900),
         static_cast<int8_t>(localTime.tm_mon + 1),
         static_cast<int8_t>(localTime.tm_mday)},
        {static_cast<int8_t>(localTime.tm_hour),
         static_cast<int8_t>(localTime.tm_min),
         static_cast<int8_t>(localTime.tm_sec)}});
  }

  return true;
}

}  // namespace iris
