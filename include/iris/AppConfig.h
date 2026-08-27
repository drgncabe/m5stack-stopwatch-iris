#pragma once

#include <Arduino.h>

namespace iris::config {

constexpr char kProjectName[] = "Iris";
constexpr char kVersion[] = "0.3.8";

// POSIX timezone string for US Eastern time. Change this in later deployments
// if the device should operate in another timezone.
constexpr char kTimezone[] = "EST5EDT,M3.2.0,M11.1.0";
constexpr char kNtpServer1[] = "0.pool.ntp.org";
constexpr char kNtpServer2[] = "1.pool.ntp.org";
constexpr char kNtpServer3[] = "2.pool.ntp.org";

constexpr uint8_t kDefaultVolume = 96;
constexpr uint8_t kActiveBrightness = 96;
constexpr uint8_t kDimBrightness = 18;
constexpr uint32_t kDisplayDimMs = 20UL * 1000UL;
constexpr uint32_t kDisplaySleepMs = 90UL * 1000UL;
constexpr uint32_t kWifiReconnectMs = 15000;
constexpr uint32_t kWifiOnDemandOffMs = 2UL * 60UL * 1000UL;
constexpr uint32_t kWifiPortalTimeoutMs = 10UL * 60UL * 1000UL;
constexpr uint32_t kNtpRetryMs = 30000;
constexpr uint32_t kNtpResyncMs = 6UL * 60UL * 60UL * 1000UL;

}  // namespace iris::config
