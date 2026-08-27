#include "iris/services/NetworkScanService.h"

#include <WiFi.h>

namespace iris {

namespace {
constexpr uint32_t kActiveScanMaxMsPerChannel = 450;

void restoreScannerWifiState(WifiService& wifi,
                             bool radioTemporarilyEnabled,
                             bool stationWasConnected) {
  if (radioTemporarilyEnabled) {
    WiFi.mode(WIFI_OFF);
    return;
  }

  WiFi.setSleep(true);
  if (stationWasConnected) {
    Serial.println("[SCAN] reconnecting saved WiFi after scan");
    wifi.reconnectSaved();
  }
}
}  // namespace

NetworkScanService::NetworkScanService(WifiService& wifi) : wifi_(wifi) {}

void NetworkScanService::begin() {
  clearResults();
}

void NetworkScanService::update(uint32_t nowMs) {
  if (state_ != NetworkScanState::Scanning) return;

  const int status = WiFi.scanComplete();
  lastDriverStatus_ = status;
  if (status == WIFI_SCAN_RUNNING) return;
  if (status >= 0) {
    Serial.printf("[SCAN] complete count=%d elapsed=%lums\n", status,
                  static_cast<unsigned long>(nowMs - scanStartedMs_));
    finishScan(status, nowMs);
    return;
  }

  Serial.printf("[SCAN] failed status=%d elapsed=%lums mode=%d connected=%d\n", status,
                static_cast<unsigned long>(nowMs - scanStartedMs_),
                static_cast<int>(WiFi.getMode()), WiFi.status() == WL_CONNECTED ? 1 : 0);
  state_ = NetworkScanState::Failed;
  WiFi.scanDelete();
  restoreScannerWifiState(wifi_, radioTemporarilyEnabled_, stationWasConnected_);
  radioTemporarilyEnabled_ = false;
  stationWasConnected_ = false;
}

bool NetworkScanService::startScan() {
  if (state_ == NetworkScanState::Scanning) {
    Serial.println("[SCAN] start ignored: scan already running");
    return false;
  }
  if (wifi_.isProvisioning()) {
    Serial.println("[SCAN] start blocked: WiFi setup portal is active");
    state_ = NetworkScanState::Unavailable;
    return false;
  }

  WiFi.scanDelete();
  clearResults();
  stationWasConnected_ = wifi_.isConnected();
  radioTemporarilyEnabled_ = !wifi_.isEnabled();
  if (stationWasConnected_) {
    Serial.printf("[SCAN] disconnecting station for scan ssid=%s\n", WiFi.SSID().c_str());
    WiFi.disconnect(false, false);
    delay(120);
  }
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  delay(30);

  const int started = WiFi.scanNetworks(true, true, false, kActiveScanMaxMsPerChannel);
  lastDriverStatus_ = started;
  scanStartedMs_ = millis();
  Serial.printf("[SCAN] start requested result=%d tempRadio=%d wasConnected=%d mode=%d connected=%d maxChanMs=%lu\n",
                started, radioTemporarilyEnabled_ ? 1 : 0, stationWasConnected_ ? 1 : 0,
                static_cast<int>(WiFi.getMode()), WiFi.status() == WL_CONNECTED ? 1 : 0,
                static_cast<unsigned long>(kActiveScanMaxMsPerChannel));
  if (started == WIFI_SCAN_FAILED) {
    Serial.println("[SCAN] start failed: WiFi driver rejected scan request");
    state_ = NetworkScanState::Failed;
    restoreScannerWifiState(wifi_, radioTemporarilyEnabled_, stationWasConnected_);
    radioTemporarilyEnabled_ = false;
    stationWasConnected_ = false;
    return false;
  }

  state_ = NetworkScanState::Scanning;
  return true;
}

void NetworkScanService::cancelScan() {
  if (state_ == NetworkScanState::Scanning) {
    Serial.println("[SCAN] cancelled");
    WiFi.scanDelete();
  }
  if (radioTemporarilyEnabled_) {
    WiFi.scanDelete();
  }
  restoreScannerWifiState(wifi_, radioTemporarilyEnabled_, stationWasConnected_);
  radioTemporarilyEnabled_ = false;
  stationWasConnected_ = false;
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
  Serial.printf("[SCAN] processing detected=%d limit=%u\n", detectedCount_,
                static_cast<unsigned>(kMaxResults));

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
    Serial.printf("[SCAN] #%d rssi=%ld ch=%u security=%s hidden=%d bssid=%s ssid=%s\n",
                  i + 1, static_cast<long>(result.rssi), result.channel,
                  securityName(result.security), result.hidden ? 1 : 0, result.bssid.c_str(),
                  result.ssid.c_str());
  }

  WiFi.scanDelete();
  restoreScannerWifiState(wifi_, radioTemporarilyEnabled_, stationWasConnected_);
  radioTemporarilyEnabled_ = false;
  stationWasConnected_ = false;
  lastScanMs_ = nowMs;
  state_ = NetworkScanState::Complete;
  Serial.printf("[SCAN] stored=%u truncated=%d strongest=%ld\n",
                static_cast<unsigned>(resultCount_), truncated() ? 1 : 0,
                static_cast<long>(strongestRssi_));
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
