#pragma once

#include <Arduino.h>

#include "iris/services/SettingsStore.h"

namespace iris {

struct DateTimeSnapshot {
  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  int second = 0;
  int weekDay = 0;
  bool valid = false;
};

class TimeService {
 public:
  explicit TimeService(SettingsStore& settings) : settings_(settings) {}

  void begin();
  void update(uint32_t nowMs, bool wifiConnected);
  DateTimeSnapshot now() const;
  bool rtcAvailable() const { return rtcAvailable_; }
  bool ntpSynchronized() const { return ntpSynchronized_; }
  bool syncNow(uint32_t nowMs);
  bool adjustManualMinutes(int deltaMinutes);
  void applyConfiguredTimezone();
  String formatDate(const DateTimeSnapshot& value) const;
  String formatTime(const DateTimeSnapshot& value, bool includeSeconds = false) const;
  String formatDateTime(const DateTimeSnapshot& value) const;
  String utcOffsetText() const;
  String dstText() const;
  String lastNtpSyncText() const;

 private:
  void requestNtp(uint32_t nowMs);
  bool copySystemTimeToRtc();
  bool restoreSystemTimeFromRtc();
  bool setSystemAndRtc(time_t epoch);

  SettingsStore& settings_;
  bool rtcAvailable_ = false;
  bool ntpRequested_ = false;
  bool ntpSynchronized_ = false;
  uint32_t lastNtpRequestMs_ = 0;
  uint32_t lastNtpSyncMs_ = 0;
  time_t lastNtpSyncEpoch_ = 0;
};

}  // namespace iris
