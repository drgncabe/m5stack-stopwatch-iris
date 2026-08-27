#include "iris/services/NetworkScanService.h"

#include <WiFi.h>

namespace iris {

NetworkScanService::NetworkScanService(WifiService& wifi) : wifi_(wifi) {}

void NetworkScanService::begin() {
  clearResults();
}

void NetworkScanService::update(uint32_t nowMs) {
  if (state_ != NetworkScanState::Scanning) return;

  const int status = WiFi.scanComplete();
  if (status == WIFI_SCAN_RUNNING) return;
  if (status >= 0) {
    finishScan(status, nowMs);
    return;
  }

  state_ = NetworkScanState::Failed;
  if (radioTemporarilyEnabled_) {
    WiFi.scanDelete();
    WiFi.mode(WIFI_OFF);
    radioTemporarilyEnabled_ = false;
  }
}

bool NetworkScanService::startScan() {
  if (state_ == NetworkScanState::Scanning) return false;
  if (wifi_.isProvisioning()) {
    state_ = NetworkScanState::Unavailable;
    return false;
  }

  WiFi.scanDelete();
  clearResults();
  radioTemporarilyEnabled_ = !wifi_.isEnabled();
  if (radioTemporarilyEnabled_) {
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
  }

  const int started = WiFi.scanNetworks(true, true);
  if (started == WIFI_SCAN_FAILED) {
    state_ = NetworkScanState::Failed;
    if (radioTemporarilyEnabled_) {
      WiFi.mode(WIFI_OFF);
      radioTemporarilyEnabled_ = false;
    }
    return false;
  }

  state_ = NetworkScanState::Scanning;
  return true;
}

void NetworkScanService::cancelScan() {
  if (state_ == NetworkScanState::Scanning) {
    WiFi.scanDelete();
  }
  if (radioTemporarilyEnabled_) {
    WiFi.mode(WIFI_OFF);
    radioTemporarilyEnabled_ = false;
  }
  state_ = NetworkScanState::Idle;
}

const char* NetworkScanService::stateText() const {
  switch (state_) {
    case NetworkScanState::Idle: return "Ready";
    case NetworkScanState::Scanning: return "Scanning...";
    case NetworkScanState::Complete: return "Complete";
    case NetworkScanState::Failed: return "Scan failed";
    case NetworkScanState::Unavailable: return "Unavailable";
    default: return "Unknown";
  }
}

const WifiScanResult* NetworkScanService::resultAt(size_t index) const {
  return index < resultCount_ ? &results_[index] : nullptr;
}

uint8_t NetworkScanService::channelUse(uint8_t channel) const {
  return channel < sizeof(channelUse_) ? channelUse_[channel] : 0;
}

const char* NetworkScanService::securityName(WifiSecurity security) {
  switch (security) {
    case WifiSecurity::Open: return "Open";
    case WifiSecurity::Wep: return "WEP";
    case WifiSecurity::Wpa: return "WPA";
    case WifiSecurity::Wpa2: return "WPA2";
    case WifiSecurity::WpaWpa2: return "WPA/WPA2";
    case WifiSecurity::Wpa2Enterprise: return "WPA2 Ent";
    case WifiSecurity::Wpa3: return "WPA3";
    case WifiSecurity::Wpa2Wpa3: return "WPA2/WPA3";
    default: return "Unknown";
  }
}

const char* NetworkScanService::bandName(WifiBand band) {
  switch (band) {
    case WifiBand::Band24GHz: return "2.4 GHz";
    default: return "Unknown";
  }
}

void NetworkScanService::finishScan(int count, uint32_t nowMs) {
  clearResults();
  detectedCount_ = count > 0 ? count : 0;
  strongestRssi_ = detectedCount_ > 0 ? WiFi.RSSI(0) : 0;

  for (int i = 0; i < detectedCount_; ++i) {
    WifiScanResult result;
    result.ssid = WiFi.SSID(i);
    result.hidden = result.ssid.length() == 0;
    if (result.hidden) result.ssid = "<Hidden Network>";
    result.bssid = WiFi.BSSIDstr(i);
    result.rssi = WiFi.RSSI(i);
    result.channel = static_cast<uint8_t>(WiFi.channel(i));
    result.security = securityFromAuth(WiFi.encryptionType(i));
    result.band = WifiBand::Band24GHz;
    if (result.channel < sizeof(channelUse_)) channelUse_[result.channel]++;
    if (i == 0 || result.rssi > strongestRssi_) strongestRssi_ = result.rssi;
    insertSorted(result);
  }

  WiFi.scanDelete();
  if (radioTemporarilyEnabled_) {
    WiFi.mode(WIFI_OFF);
    radioTemporarilyEnabled_ = false;
  }
  lastScanMs_ = nowMs;
  state_ = NetworkScanState::Complete;
}

void NetworkScanService::clearResults() {
  for (size_t i = 0; i < resultCount_; ++i) {
    results_[i].ssid = "";
    results_[i].bssid = "";
  }
  resultCount_ = 0;
  detectedCount_ = 0;
  strongestRssi_ = 0;
  memset(channelUse_, 0, sizeof(channelUse_));
}

void NetworkScanService::insertSorted(const WifiScanResult& result) {
  size_t insertAt = resultCount_;
  for (size_t i = 0; i < resultCount_; ++i) {
    if (result.rssi > results_[i].rssi) {
      insertAt = i;
      break;
    }
  }

  if (resultCount_ < kMaxResults) {
    for (size_t i = resultCount_; i > insertAt; --i) {
      results_[i] = results_[i - 1];
    }
    results_[insertAt] = result;
    resultCount_++;
  } else if (insertAt < kMaxResults) {
    for (size_t i = kMaxResults - 1; i > insertAt; --i) {
      results_[i] = results_[i - 1];
    }
    results_[insertAt] = result;
  }
}

WifiSecurity NetworkScanService::securityFromAuth(wifi_auth_mode_t type) {
  switch (type) {
    case WIFI_AUTH_OPEN: return WifiSecurity::Open;
    case WIFI_AUTH_WEP: return WifiSecurity::Wep;
    case WIFI_AUTH_WPA_PSK: return WifiSecurity::Wpa;
    case WIFI_AUTH_WPA2_PSK: return WifiSecurity::Wpa2;
    case WIFI_AUTH_WPA_WPA2_PSK: return WifiSecurity::WpaWpa2;
    case WIFI_AUTH_WPA2_ENTERPRISE: return WifiSecurity::Wpa2Enterprise;
    case WIFI_AUTH_WPA3_PSK: return WifiSecurity::Wpa3;
    case WIFI_AUTH_WPA2_WPA3_PSK: return WifiSecurity::Wpa2Wpa3;
    default: return WifiSecurity::Unknown;
  }
}

}  // namespace iris
