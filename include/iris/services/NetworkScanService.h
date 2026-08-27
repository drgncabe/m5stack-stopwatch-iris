#pragma once

#include <Arduino.h>

#include "iris/services/WifiService.h"

namespace iris {

enum class WifiSecurity : uint8_t {
  Open,
  Wep,
  Wpa,
  Wpa2,
  WpaWpa2,
  Wpa2Enterprise,
  Wpa3,
  Wpa2Wpa3,
  Unknown,
};

enum class WifiBand : uint8_t {
  Band24GHz,
};

enum class NetworkScanState : uint8_t {
  Idle,
  Scanning,
  Complete,
  Failed,
  Unavailable,
};

struct WifiScanResult {
  String ssid;
  String bssid;
  int32_t rssi = 0;
  uint8_t channel = 0;
  WifiSecurity security = WifiSecurity::Unknown;
  WifiBand band = WifiBand::Band24GHz;
  bool hidden = false;
};

class NetworkScanService {
 public:
  static constexpr size_t kMaxResults = 50;

  explicit NetworkScanService(WifiService& wifi);

  void begin();
  void update(uint32_t nowMs);
  bool startScan();
  void cancelScan();

  NetworkScanState state() const { return state_; }
  const char* stateText() const;
  const char* sourceName() const { return "Internal ESP32-S3"; }
  const char* bandName() const { return "2.4 GHz"; }
  bool scanning() const { return state_ == NetworkScanState::Scanning; }
  bool truncated() const { return detectedCount_ > resultCount_; }
  size_t resultCount() const { return resultCount_; }
  int detectedCount() const { return detectedCount_; }
  int32_t strongestRssi() const { return strongestRssi_; }
  uint32_t lastScanMs() const { return lastScanMs_; }
  int lastDriverStatus() const { return lastDriverStatus_; }
  const WifiScanResult* resultAt(size_t index) const;
  uint8_t channelUse(uint8_t channel) const;

  static const char* securityName(WifiSecurity security);
  static const char* bandName(WifiBand band);

 private:
  void finishScan(int count, uint32_t nowMs);
  void clearResults();
  void insertSorted(const WifiScanResult& result);
  static WifiSecurity securityFromAuth(wifi_auth_mode_t type);

  WifiService& wifi_;
  NetworkScanState state_ = NetworkScanState::Idle;
  WifiScanResult results_[kMaxResults]{};
  size_t resultCount_ = 0;
  int detectedCount_ = 0;
  int32_t strongestRssi_ = 0;
  uint8_t channelUse_[15]{};
  uint32_t lastScanMs_ = 0;
  uint32_t scanStartedMs_ = 0;
  int lastDriverStatus_ = 0;
  bool radioTemporarilyEnabled_ = false;
  bool stationWasConnected_ = false;
};

}  // namespace iris
