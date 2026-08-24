#include "iris/services/TimeService.h"

#include <M5Unified.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "iris/AppConfig.h"

namespace iris {

namespace {
constexpr const char* kMonthNames[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
constexpr uint32_t kNtpSettleMs = 1500;
constexpr uint32_t kNtpTimeoutMs = 15000;

bool snapshotLooksValid(const DateTimeSnapshot& snapshot) {
  return snapshot.year >= 2024 &&
         snapshot.month >= 1 && snapshot.month <= 12 &&
         snapshot.day >= 1 && snapshot.day <= 31 &&
         snapshot.hour >= 0 && snapshot.hour <= 23 &&
         snapshot.minute >= 0 && snapshot.minute <= 59 &&
         snapshot.second >= 0 && snapshot.second <= 59;
}
}  // namespace

void TimeService::begin() {
  rtcAvailable_ = M5.Rtc.isEnabled();
  lastNtpSyncEpoch_ = static_cast<time_t>(settings_.lastNtpSyncEpoch());
  applyConfiguredTimezone();
  if (!restoreSystemTimeFromRtc() && events_) {
    events_->publish(EventType::RtcTimeInvalid, "TimeService", rtcAvailable_ ? "Invalid" : "Unavailable");
  }
  Serial.printf("Iris RTC: %s\n", rtcAvailable_ ? "available" : "not available");
}

void TimeService::update(uint32_t nowMs, bool wifiConnected) {
  applyConfiguredTimezone();
  const bool syncAllowed = settings_.automaticTimeEnabled() || manualSyncRequested_;
  if (!syncAllowed) return;

  if (!wifiConnected) {
    if (manualSyncRequested_) syncState_ = TimeSyncState::WaitingForWifi;
    return;
  }

  const bool resyncDue = ntpSynchronized_ &&
      (nowMs - lastNtpSyncMs_ >= config::kNtpResyncMs);
  const bool manualNeedsRequest = manualSyncRequested_ &&
      (!ntpRequested_ || syncState_ == TimeSyncState::WaitingForWifi);
  const bool retryDue = !ntpSynchronized_ &&
      (lastNtpRequestMs_ == 0 || nowMs - lastNtpRequestMs_ >= config::kNtpRetryMs);

  if (manualNeedsRequest || resyncDue || (!ntpRequested_ && retryDue)) {
    requestNtp(nowMs);
  }

  if (ntpRequested_ && copySystemTimeToRtc()) {
    ntpSynchronized_ = true;
    manualSyncRequested_ = false;
    ntpRequested_ = false;
    syncState_ = TimeSyncState::Synchronized;
    lastNtpSyncMs_ = nowMs;
    lastNtpSyncEpoch_ = time(nullptr);
    if (lastNtpSyncEpoch_ > 1700000000) {
      settings_.setLastNtpSyncEpoch(static_cast<uint32_t>(lastNtpSyncEpoch_));
    }
    if (events_) events_->publish(EventType::TimeSynchronized, "TimeService", "NTP");
  } else if (ntpRequested_ && nowMs - lastNtpRequestMs_ >= kNtpTimeoutMs) {
    syncState_ = TimeSyncState::Failed;
    manualSyncRequested_ = false;
    ntpRequested_ = false;
    if (events_) events_->publish(EventType::TimeSyncFailed, "TimeService", "NTP timeout");
  }
}

DateTimeSnapshot TimeService::now() const {
  DateTimeSnapshot snapshot = systemNow();
  if (snapshot.valid) return snapshot;

  if (rtcAvailable_) {
    snapshot = rtcNow();
  }

  return snapshot;
}

DateTimeSnapshot TimeService::systemNow() const {
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

  return snapshot;
}

DateTimeSnapshot TimeService::rtcNow() const {
  DateTimeSnapshot snapshot;
  if (!rtcAvailable_) return snapshot;

  const auto dt = M5.Rtc.getDateTime();
  snapshot.year = dt.date.year;
  snapshot.month = dt.date.month;
  snapshot.day = dt.date.date;
  snapshot.hour = dt.time.hours;
  snapshot.minute = dt.time.minutes;
  snapshot.second = dt.time.seconds;
  snapshot.weekDay = dt.date.weekDay;
  snapshot.valid = snapshotLooksValid(snapshot);
  return snapshot;
}

void TimeService::requestNtp(uint32_t nowMs) {
  configTzTime(timeZonePosix(settings_.timeZone()),
               config::kNtpServer1,
               config::kNtpServer2,
               config::kNtpServer3);
  ntpRequested_ = true;
  syncState_ = TimeSyncState::Requested;
  lastNtpRequestMs_ = nowMs;
  Serial.println("Iris time: NTP synchronization requested");
}

bool TimeService::copySystemTimeToRtc() {
  if (millis() - lastNtpRequestMs_ < kNtpSettleMs) return false;

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

bool TimeService::syncNow(uint32_t nowMs) {
  manualSyncRequested_ = true;
  ntpRequested_ = false;
  syncState_ = TimeSyncState::WaitingForWifi;
  lastNtpRequestMs_ = nowMs;
  Serial.println("Iris time: manual NTP synchronization queued");
  return true;
}

bool TimeService::adjustManualMinutes(int deltaMinutes) {
  if (settings_.automaticTimeEnabled()) return false;

  const DateTimeSnapshot current = now();
  if (!snapshotLooksValid(current)) return false;

  struct tm localTime {};
  localTime.tm_year = current.year - 1900;
  localTime.tm_mon = current.month - 1;
  localTime.tm_mday = current.day;
  localTime.tm_hour = current.hour;
  localTime.tm_min = current.minute;
  localTime.tm_sec = current.second;
  localTime.tm_isdst = -1;

  time_t epoch = mktime(&localTime);
  if (epoch <= 0) return false;
  epoch += static_cast<time_t>(deltaMinutes) * 60;
  ntpSynchronized_ = false;
  const bool saved = setSystemAndRtc(epoch);
  if (saved && events_) events_->publish(EventType::TimeChanged, "TimeService", "Manual adjust");
  return saved;
}

bool TimeService::setManualDateTime(const DateTimeSnapshot& value) {
  if (settings_.automaticTimeEnabled()) return false;
  if (!snapshotLooksValid(value)) return false;

  struct tm localTime {};
  localTime.tm_year = value.year - 1900;
  localTime.tm_mon = value.month - 1;
  localTime.tm_mday = value.day;
  localTime.tm_hour = value.hour;
  localTime.tm_min = value.minute;
  localTime.tm_sec = value.second;
  localTime.tm_isdst = -1;

  const time_t epoch = mktime(&localTime);
  if (epoch <= 0) return false;
  ntpSynchronized_ = false;
  const bool saved = setSystemAndRtc(epoch);
  if (saved && events_) events_->publish(EventType::TimeChanged, "TimeService", "Manual");
  return saved;
}

bool TimeService::setManualDateTimeText(const String& value) {
  DateTimeSnapshot snapshot;
  int matched = sscanf(value.c_str(), "%d-%d-%dT%d:%d:%d",
                       &snapshot.year,
                       &snapshot.month,
                       &snapshot.day,
                       &snapshot.hour,
                       &snapshot.minute,
                       &snapshot.second);
  if (matched < 5) {
    snapshot.second = 0;
    matched = sscanf(value.c_str(), "%d-%d-%dT%d:%d",
                     &snapshot.year,
                     &snapshot.month,
                     &snapshot.day,
                     &snapshot.hour,
                     &snapshot.minute);
  }
  if (matched < 5) return false;
  snapshot.valid = true;
  return setManualDateTime(snapshot);
}

void TimeService::applyConfiguredTimezone() {
  setenv("TZ", timeZonePosix(settings_.timeZone()), 1);
  tzset();
}

String TimeService::formatDate(const DateTimeSnapshot& value) const {
  if (!value.valid) return "--";

  char buffer[40];
  const char* month = kMonthNames[(value.month - 1) % 12];
  switch (settings_.dateFormat()) {
    case DateFormat::DayMonthYear:
      snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d", value.day, value.month, value.year);
      break;
    case DateFormat::YearMonthDay:
      snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", value.year, value.month, value.day);
      break;
    case DateFormat::MonthNameDay:
      snprintf(buffer, sizeof(buffer), "%s %d, %04d", month, value.day, value.year);
      break;
    case DateFormat::DayMonthName:
      snprintf(buffer, sizeof(buffer), "%d %s %04d", value.day, month, value.year);
      break;
    default:
      snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d", value.month, value.day, value.year);
      break;
  }
  return String(buffer);
}

String TimeService::formatTime(const DateTimeSnapshot& value, bool includeSeconds) const {
  if (!value.valid) return "--:--";

  char buffer[24];
  if (settings_.timeFormat() == TimeFormat::TwentyFourHour) {
    if (includeSeconds) {
      snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", value.hour, value.minute, value.second);
    } else {
      snprintf(buffer, sizeof(buffer), "%02d:%02d", value.hour, value.minute);
    }
    return String(buffer);
  }

  int hour = value.hour % 12;
  if (hour == 0) hour = 12;
  const char* suffix = value.hour >= 12 ? "PM" : "AM";
  if (includeSeconds) {
    snprintf(buffer, sizeof(buffer), "%d:%02d:%02d %s", hour, value.minute, value.second, suffix);
  } else {
    snprintf(buffer, sizeof(buffer), "%d:%02d %s", hour, value.minute, suffix);
  }
  return String(buffer);
}

String TimeService::formatDateTime(const DateTimeSnapshot& value) const {
  if (!value.valid) return "Not set";
  return formatDate(value) + " " + formatTime(value, true);
}

String TimeService::utcOffsetText() const {
  time_t systemNow = time(nullptr);
  if (systemNow <= 1700000000) return "Unknown";

  struct tm localTime {};
  localtime_r(&systemNow, &localTime);
  char offset[8] {};
  if (strftime(offset, sizeof(offset), "%z", &localTime) == 0) return "Unknown";
  if (strlen(offset) != 5) return String("UTC") + offset;

  String text("UTC");
  text += offset[0];
  text += offset[1];
  text += offset[2];
  text += ":";
  text += offset[3];
  text += offset[4];
  return text;
}

String TimeService::dstText() const {
  time_t systemNow = time(nullptr);
  if (systemNow <= 1700000000) return "Unknown";

  struct tm localTime {};
  localtime_r(&systemNow, &localTime);
  if (localTime.tm_isdst > 0) return "Active";
  if (localTime.tm_isdst == 0) return "Inactive";
  return "Unknown";
}

String TimeService::syncStatusText() const {
  switch (syncState_) {
    case TimeSyncState::WaitingForWifi:
      return "Waiting for WiFi";
    case TimeSyncState::Requested:
      return "Sync requested";
    case TimeSyncState::Synchronized:
      return "Synchronized";
    case TimeSyncState::Failed:
      return "Sync failed";
    default:
      break;
  }

  if (ntpSynchronized_) return "Synchronized";
  return settings_.automaticTimeEnabled() ? "Not synced" : "Manual time";
}

String TimeService::lastNtpSyncText() const {
  if (lastNtpSyncEpoch_ <= 1700000000) return "Never";

  struct tm localTime {};
  localtime_r(&lastNtpSyncEpoch_, &localTime);
  DateTimeSnapshot snapshot;
  snapshot.year = localTime.tm_year + 1900;
  snapshot.month = localTime.tm_mon + 1;
  snapshot.day = localTime.tm_mday;
  snapshot.hour = localTime.tm_hour;
  snapshot.minute = localTime.tm_min;
  snapshot.second = localTime.tm_sec;
  snapshot.weekDay = localTime.tm_wday;
  snapshot.valid = true;
  return formatDate(snapshot) + " " + formatTime(snapshot);
}

String TimeService::rtcSystemDifferenceText() const {
  const DateTimeSnapshot rtc = rtcNow();
  const DateTimeSnapshot system = systemNow();
  if (!rtc.valid || !system.valid) return "Unknown";

  time_t rtcEpoch = 0;
  time_t systemEpoch = 0;
  if (!snapshotToEpoch(rtc, &rtcEpoch) || !snapshotToEpoch(system, &systemEpoch)) {
    return "Unknown";
  }

  long diff = static_cast<long>(difftime(rtcEpoch, systemEpoch));
  const char sign = diff >= 0 ? '+' : '-';
  if (diff < 0) diff = -diff;

  String text;
  text.reserve(12);
  text += sign;
  text += String(diff);
  text += " sec";
  return text;
}

bool TimeService::restoreSystemTimeFromRtc() {
  if (!rtcAvailable_) return false;

  const DateTimeSnapshot snapshot = rtcNow();
  if (!snapshot.valid) return false;

  time_t epoch = 0;
  if (!snapshotToEpoch(snapshot, &epoch)) return false;
  return setSystemAndRtc(epoch);
}

bool TimeService::setSystemAndRtc(time_t epoch) {
  timeval tv {};
  tv.tv_sec = epoch;
  settimeofday(&tv, nullptr);

  struct tm localTime {};
  localtime_r(&epoch, &localTime);
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

bool TimeService::snapshotToEpoch(const DateTimeSnapshot& snapshot, time_t* epoch) const {
  if (!epoch || !snapshotLooksValid(snapshot)) return false;

  struct tm localTime {};
  localTime.tm_year = snapshot.year - 1900;
  localTime.tm_mon = snapshot.month - 1;
  localTime.tm_mday = snapshot.day;
  localTime.tm_hour = snapshot.hour;
  localTime.tm_min = snapshot.minute;
  localTime.tm_sec = snapshot.second;
  localTime.tm_isdst = -1;

  const time_t value = mktime(&localTime);
  if (value <= 0) return false;
  *epoch = value;
  return true;
}

}  // namespace iris
