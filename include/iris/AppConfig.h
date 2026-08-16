#pragma once

#include <Arduino.h>

namespace iris::config {

inline constexpr char kProjectName[] = "Iris";
inline constexpr char kVersion[] = "0.1.0-phase1";

// POSIX timezone string for US Eastern time. Change this in later deployments
// if the device should operate in another timezone.
inline constexpr char kTimezone[] = "EST5EDT,M3.2.0,M11.1.0";
inline constexpr char kNtpServer1[] = "0.pool.ntp.org";
inline constexpr char kNtpServer2[] = "1.pool.ntp.org";
inline constexpr char kNtpServer3[] = "2.pool.ntp.org";

inline constexpr uint8_t kDefaultVolume = 96;
inline constexpr uint32_t kWifiReconnectMs = 15000;
inline constexpr uint32_t kNtpRetryMs = 30000;
inline constexpr uint32_t kNtpResyncMs = 6UL * 60UL * 60UL * 1000UL;

}  // namespace iris::config
