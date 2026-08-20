#pragma once

#include <Arduino.h>

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
  void begin();
  void update(uint32_t nowMs, bool wifiConnected);
  DateTimeSnapshot now() const;
  bool rtcAvailable() const { return rtcAvailable_; }
  bool ntpSynchronized() const { return ntpSynchronized_; }

 private:
  void requestNtp(uint32_t nowMs);
  bool copySystemTimeToRtc();

  bool rtcAvailable_ = false;
  bool ntpRequested_ = false;
  bool ntpSynchronized_ = false;
  uint32_t lastNtpRequestMs_ = 0;
  uint32_t lastNtpSyncMs_ = 0;
};

}  // namespace iris
