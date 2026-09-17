#include "iris/services/RagnarLinkService.h"

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

namespace iris {

namespace {
constexpr uint32_t kRagnarStaleMs = 15000;
constexpr uint8_t kPacketLength = 24;
constexpr uint8_t kPacketVersion = 1;
constexpr uint8_t kStatusPacketType = 0x01;

RagnarLinkService* sService = nullptr;

uint16_t readLe16(const uint8_t* data) {
  return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
}

uint32_t readLe32(const uint8_t* data) {
  return static_cast<uint32_t>(data[0]) |
         (static_cast<uint32_t>(data[1]) << 8) |
         (static_cast<uint32_t>(data[2]) << 16) |
         (static_cast<uint32_t>(data[3]) << 24);
}

uint16_t crc16CcittFalse(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; ++i) {
    crc ^= static_cast<uint16_t>(data[i]) << 8;
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021)
                           : static_cast<uint16_t>(crc << 1);
    }
  }
  return crc;
}

uint8_t normalizedChannel(uint8_t channel) {
  return constrain(channel, 1, 14);
}

String formatMac(const uint8_t* mac) {
  if (!mac) return "Unknown";
  char buffer[18];
  snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buffer);
}
}  // namespace

void RagnarLinkService::begin(uint8_t channel, bool enabled) {
  snapshot_.configuredChannel = normalizedChannel(channel);
  snapshot_.enabled = enabled;
  sService = this;
  if (!snapshot_.enabled) return;
  ensureWifi(snapshot_.configuredChannel, WiFi.status() == WL_CONNECTED, false);
  ensureEspNow();
}

void RagnarLinkService::update(uint32_t nowMs, uint8_t configuredChannel, bool wifiConnected,
                               bool enabled, bool provisioning) {
  snapshot_.configuredChannel = normalizedChannel(configuredChannel);
  setEnabled(enabled);
  snapshot_.stale = stale(nowMs);
  if (!snapshot_.enabled) return;
  ensureWifi(snapshot_.configuredChannel, wifiConnected, provisioning);
  ensureEspNow();
}

void RagnarLinkService::setEnabled(bool enabled) {
  if (snapshot_.enabled == enabled) return;
  snapshot_.enabled = enabled;
  if (!snapshot_.enabled) {
    stopEspNow();
    snapshot_.initialized = false;
    snapshot_.packetSeen = false;
    snapshot_.stale = true;
    snapshot_.activeChannel = 0;
  }
}

RagnarLinkSnapshot RagnarLinkService::snapshot() const {
  RagnarLinkSnapshot copy = snapshot_;
  copy.stale = stale(millis());
  return copy;
}

bool RagnarLinkService::stale(uint32_t nowMs) const {
  if (!snapshot_.packetSeen) return true;
  return nowMs - snapshot_.lastPacketMs > kRagnarStaleMs;
}

const char* RagnarLinkService::statusText(uint32_t nowMs) const {
  if (!snapshot_.enabled) return "Disabled";
  if (!snapshot_.initialized) return "Unavailable";
  if (!snapshot_.packetSeen) return "Disconnected";
  if (stale(nowMs)) return "Stale";
  return "Connected";
}

const char* RagnarLinkService::ragnarStateName(RagnarState state) {
  switch (state) {
    case RagnarState::Idle: return "idle";
    case RagnarState::Active: return "active";
    case RagnarState::Error: return "error";
    default: return "unknown";
  }
}

const char* RagnarLinkService::gpsStateName(RagnarGpsState state) {
  switch (state) {
    case RagnarGpsState::NoFix: return "no fix";
    case RagnarGpsState::Fix: return "fix";
    default: return "unknown";
  }
}

const char* RagnarLinkService::captureStateName(RagnarCaptureState state) {
  switch (state) {
    case RagnarCaptureState::Running: return "running";
    case RagnarCaptureState::Paused: return "paused";
    case RagnarCaptureState::Error: return "error";
    default: return "idle";
  }
}

const char* RagnarLinkService::dropReasonName(RagnarDropReason reason) {
  switch (reason) {
    case RagnarDropReason::None: return "none";
    case RagnarDropReason::WrongLength: return "wrong length";
    case RagnarDropReason::BadMagic: return "bad magic";
    case RagnarDropReason::BadVersion: return "bad version";
    case RagnarDropReason::BadType: return "bad type";
    case RagnarDropReason::BadCrc: return "bad CRC";
    default: return "unknown";
  }
}

void RagnarLinkService::onReceive(const uint8_t* mac, const uint8_t* data, int len) {
  if (sService) sService->handleReceive(mac, data, len);
}

void RagnarLinkService::handleReceive(const uint8_t* mac, const uint8_t* data, int len) {
  snapshot_.rawRxPackets++;
  snapshot_.lastSenderMac = formatMac(mac);
  snapshot_.lastPacketLength = len;
  Serial.printf("[Ragnar] ESP-NOW RX raw=%lu from=%s len=%d channel=%u initialized=%s\n",
                static_cast<unsigned long>(snapshot_.rawRxPackets),
                snapshot_.lastSenderMac.c_str(),
                len,
                snapshot_.activeChannel,
                snapshot_.initialized ? "yes" : "no");

  RagnarLinkSnapshot parsed = snapshot_;
  RagnarDropReason reason = RagnarDropReason::None;
  if (!parseStatusPacket(data, len, &parsed, &reason)) {
    noteInvalid(reason);
    return;
  }

  parsed.packetSeen = true;
  parsed.stale = false;
  parsed.lastPacketMs = millis();
  parsed.lastDropReason = RagnarDropReason::None;
  parsed.validPackets = snapshot_.validPackets + 1;
  parsed.invalidPackets = snapshot_.invalidPackets;
  parsed.initialized = snapshot_.initialized;
  parsed.configuredChannel = snapshot_.configuredChannel;
  parsed.activeChannel = static_cast<uint8_t>(WiFi.channel());
  snapshot_ = parsed;
  Serial.printf("[Ragnar] valid=%lu seq=%lu age=0s from=%s len=%d channel=%u\n",
                static_cast<unsigned long>(snapshot_.validPackets),
                static_cast<unsigned long>(snapshot_.sequence),
                snapshot_.lastSenderMac.c_str(),
                snapshot_.lastPacketLength,
                snapshot_.activeChannel);
}

bool RagnarLinkService::parseStatusPacket(const uint8_t* data, int len,
                                          RagnarLinkSnapshot* out,
                                          RagnarDropReason* reason) const {
  if (reason) *reason = RagnarDropReason::None;
  if (!data || !out || len != kPacketLength) {
    if (reason) *reason = RagnarDropReason::WrongLength;
    return false;
  }
  if (data[0] != 'R' || data[1] != 'L') {
    if (reason) *reason = RagnarDropReason::BadMagic;
    return false;
  }
  if (data[2] != kPacketVersion) {
    if (reason) *reason = RagnarDropReason::BadVersion;
    return false;
  }
  if (data[3] != kStatusPacketType) {
    if (reason) *reason = RagnarDropReason::BadType;
    return false;
  }

  const uint16_t expected = crc16CcittFalse(data, 22);
  const uint16_t actual = readLe16(data + 22);
  if (expected != actual) {
    if (reason) *reason = RagnarDropReason::BadCrc;
    return false;
  }

  out->sequence = readLe32(data + 4);
  out->uptimeSeconds = readLe32(data + 8);
  out->cameraCount = readLe16(data + 12);
  out->wifiCount = readLe16(data + 14);
  out->bleCount = readLe16(data + 16);
  out->ragnarState = static_cast<RagnarState>(data[18]);
  out->gpsState = static_cast<RagnarGpsState>(data[19]);
  out->captureState = static_cast<RagnarCaptureState>(data[20]);
  out->gatewayRssi = static_cast<int8_t>(data[21]);
  return true;
}

bool RagnarLinkService::ensureWifi(uint8_t channel, bool wifiConnected, bool provisioning) {
  const wifi_mode_t mode = WiFi.getMode();
  if (mode == WIFI_OFF) {
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
  }

  if (!wifiConnected && !provisioning) {
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  }

  snapshot_.activeChannel = static_cast<uint8_t>(WiFi.channel());
  return true;
}

bool RagnarLinkService::ensureEspNow() {
  if (espNowReady_) return true;

  const esp_err_t result = esp_now_init();
  if (result == ESP_OK || result == ESP_ERR_ESPNOW_EXIST) {
    espNowReady_ = true;
    snapshot_.initialized = true;
    esp_now_register_recv_cb(RagnarLinkService::onReceive);
    Serial.printf("[Ragnar] ESP-NOW receiver ready channel=%u\n", snapshot_.activeChannel);
    return true;
  }

  snapshot_.initialized = false;
  Serial.printf("[Ragnar] ESP-NOW init failed: %d\n", static_cast<int>(result));
  return false;
}

void RagnarLinkService::stopEspNow() {
  if (!espNowReady_) return;
  esp_now_unregister_recv_cb();
  esp_now_deinit();
  espNowReady_ = false;
  Serial.println("[Ragnar] ESP-NOW receiver stopped");
}

void RagnarLinkService::noteInvalid(RagnarDropReason reason) {
  snapshot_.invalidPackets++;
  snapshot_.lastDropReason = reason;
  switch (reason) {
    case RagnarDropReason::WrongLength:
      snapshot_.wrongLengthDrops++;
      break;
    case RagnarDropReason::BadMagic:
      snapshot_.badMagicDrops++;
      break;
    case RagnarDropReason::BadVersion:
      snapshot_.badVersionDrops++;
      break;
    case RagnarDropReason::BadType:
      snapshot_.badTypeDrops++;
      break;
    case RagnarDropReason::BadCrc:
      snapshot_.badCrcDrops++;
      break;
    default:
      break;
  }
  Serial.printf("[Ragnar] drop=%s invalid=%lu raw=%lu from=%s len=%d channel=%u\n",
                dropReasonName(reason),
                static_cast<unsigned long>(snapshot_.invalidPackets),
                static_cast<unsigned long>(snapshot_.rawRxPackets),
                snapshot_.lastSenderMac.c_str(),
                snapshot_.lastPacketLength,
                snapshot_.activeChannel);
}

}  // namespace iris
