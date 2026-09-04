#pragma once

#include <Arduino.h>

namespace iris {

enum class RagnarState : uint8_t {
  Unknown = 0,
  Idle = 1,
  Active = 2,
  Error = 3,
};

enum class RagnarGpsState : uint8_t {
  Unknown = 0,
  NoFix = 1,
  Fix = 2,
};

enum class RagnarCaptureState : uint8_t {
  Idle = 0,
  Running = 1,
  Paused = 2,
  Error = 3,
};

struct RagnarLinkSnapshot {
  bool initialized = false;
  bool packetSeen = false;
  bool stale = true;
  uint8_t configuredChannel = 6;
  uint8_t activeChannel = 0;
  uint32_t lastPacketMs = 0;
  uint32_t sequence = 0;
  uint32_t uptimeSeconds = 0;
  uint16_t cameraCount = 0;
  uint16_t wifiCount = 0;
  uint16_t bleCount = 0;
  int8_t gatewayRssi = 0;
  RagnarState ragnarState = RagnarState::Unknown;
  RagnarGpsState gpsState = RagnarGpsState::Unknown;
  RagnarCaptureState captureState = RagnarCaptureState::Idle;
  uint32_t validPackets = 0;
  uint32_t invalidPackets = 0;
};

class RagnarLinkService {
 public:
  void begin(uint8_t channel);
  void update(uint32_t nowMs, uint8_t configuredChannel, bool wifiConnected, bool provisioning);

  RagnarLinkSnapshot snapshot() const;
  bool stale(uint32_t nowMs) const;
  const char* statusText(uint32_t nowMs) const;

  static const char* ragnarStateName(RagnarState state);
  static const char* gpsStateName(RagnarGpsState state);
  static const char* captureStateName(RagnarCaptureState state);

 private:
  static void onReceive(const uint8_t* mac, const uint8_t* data, int len);

  void handleReceive(const uint8_t* mac, const uint8_t* data, int len);
  bool parseStatusPacket(const uint8_t* data, int len, RagnarLinkSnapshot* out) const;
  bool ensureWifi(uint8_t channel, bool wifiConnected, bool provisioning);
  bool ensureEspNow();
  void noteInvalid();

  RagnarLinkSnapshot snapshot_;
  bool espNowReady_ = false;
};

}  // namespace iris
