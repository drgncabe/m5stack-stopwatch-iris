#pragma once

#include <Arduino.h>
#include <Preferences.h>

#include "iris/core/EventBus.h"

class NimBLEAdvertising;
class NimBLECharacteristic;
class NimBLEHIDDevice;
class NimBLEServer;
class NimBLEServerCallbacks;

namespace iris {

enum class BleMediaCommand : uint8_t {
  PlayPause,
  NextTrack,
  PreviousTrack,
  VolumeUp,
  VolumeDown,
  Mute,
};

class BluetoothService {
 public:
  bool begin();
  void update(uint32_t nowMs);
  void setEventBus(EventBus* events) { events_ = events; }

  void setEnabled(bool enabled);
  void setAutoReconnect(bool enabled);
  bool enabled() const { return enabled_; }
  bool initialized() const { return initialized_; }
  bool advertising() const { return advertising_; }
  bool connected() const { return connected_; }
  bool autoReconnect() const { return autoReconnect_; }
  bool pairingRequested() const { return pairingRequested_; }
  bool pairingFailed() const { return pairingFailed_; }
  uint32_t passkey() const { return passkey_; }
  uint32_t bondedDeviceCount() const;
  uint32_t lastCommandMs() const { return lastCommandMs_; }

  const String& deviceName() const { return deviceName_; }
  const String& activeDevice() const { return activeDevice_; }
  String statusText() const;
  String json() const;

  bool startAdvertising();
  void stopAdvertising();
  void disconnect();
  bool sendMediaCommand(BleMediaCommand command);

 private:
  class ServerCallbacks;
  class SecurityCallbacks;

  static constexpr uint32_t kKeyReleaseDelayMs = 24;
  static constexpr uint32_t kAdvertiseRestartDelayMs = 1000;

  bool initializeBle();
  void configureSecurity();
  void sendConsumerUsage(uint16_t usage);
  void releaseConsumerUsage();
  void publish(EventType type, const char* text = nullptr, int32_t value = 0);
  void handleConnected(const String& address, uint16_t connectionId);
  void handleDisconnected();
  void handlePasskey(uint32_t passkey);
  void handleAuthenticationComplete(bool success);
  uint16_t usageFor(BleMediaCommand command) const;
  const char* commandName(BleMediaCommand command) const;
  static String escapeJson(const String& value);

  Preferences prefs_;
  EventBus* events_ = nullptr;
  NimBLEServer* server_ = nullptr;
  NimBLEHIDDevice* hid_ = nullptr;
  NimBLECharacteristic* input_ = nullptr;
  NimBLEAdvertising* advertisingHandle_ = nullptr;
  ServerCallbacks* serverCallbacks_ = nullptr;
  String deviceName_;
  String activeDevice_;
  bool enabled_ = true;
  bool autoReconnect_ = true;
  bool initialized_ = false;
  bool advertising_ = false;
  bool connected_ = false;
  bool pairingRequested_ = false;
  bool pairingFailed_ = false;
  bool pendingAdvertiseRestart_ = false;
  uint16_t connectionId_ = 0;
  uint32_t passkey_ = 0;
  uint32_t lastCommandMs_ = 0;
  uint32_t releaseAtMs_ = 0;
  uint32_t restartAdvertisingAtMs_ = 0;
};

}  // namespace iris
