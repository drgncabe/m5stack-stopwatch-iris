#include "iris/services/BluetoothService.h"

#include <BLE2902.h>
#include <BLEAdvertising.h>
#include <BLEDevice.h>
#include <BLEHIDDevice.h>
#include <BLESecurity.h>
#include <BLEServer.h>
#include <esp_gap_ble_api.h>

namespace iris {

namespace {
constexpr uint8_t kReportIdConsumer = 1;
constexpr uint16_t kUsagePlayPause = 0x00CD;
constexpr uint16_t kUsageScanNext = 0x00B5;
constexpr uint16_t kUsageScanPrevious = 0x00B6;
constexpr uint16_t kUsageVolumeUp = 0x00E9;
constexpr uint16_t kUsageVolumeDown = 0x00EA;
constexpr uint16_t kUsageMute = 0x00E2;

uint8_t kConsumerReportMap[] = {
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, kReportIdConsumer,
    0x15, 0x00,        // Logical Minimum (0)
    0x26, 0xFF, 0x03,  // Logical Maximum (0x03ff)
    0x19, 0x00,        // Usage Minimum (0)
    0x2A, 0xFF, 0x03,  // Usage Maximum (0x03ff)
    0x75, 0x10,        // Report Size (16)
    0x95, 0x01,        // Report Count (1)
    0x81, 0x00,        // Input (Data, Array, Absolute)
    0xC0,              // End Collection
};

String deviceSuffix() {
  const uint64_t chipId = ESP.getEfuseMac();
  char suffix[5];
  snprintf(suffix, sizeof(suffix), "%04X", static_cast<uint16_t>(chipId & 0xFFFF));
  return String(suffix);
}
}  // namespace

class BluetoothService::ServerCallbacks : public BLEServerCallbacks {
 public:
  explicit ServerCallbacks(BluetoothService& service) : service_(service) {}

  void onConnect(BLEServer*) override {
    service_.handleConnected("", 0);
  }

  void onConnect(BLEServer*, esp_ble_gatts_cb_param_t* param) override {
    char buffer[18];
    snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X",
             param->connect.remote_bda[0], param->connect.remote_bda[1],
             param->connect.remote_bda[2], param->connect.remote_bda[3],
             param->connect.remote_bda[4], param->connect.remote_bda[5]);
    service_.handleConnected(buffer, param->connect.conn_id);
  }

  void onDisconnect(BLEServer*) override {
    service_.handleDisconnected();
  }

  void onDisconnect(BLEServer*, esp_ble_gatts_cb_param_t*) override {
    service_.handleDisconnected();
  }

 private:
  BluetoothService& service_;
};

class BluetoothService::SecurityCallbacks : public BLESecurityCallbacks {
 public:
  explicit SecurityCallbacks(BluetoothService& service) : service_(service) {}

  uint32_t onPassKeyRequest() override {
    return 0;
  }

  void onPassKeyNotify(uint32_t passKey) override {
    service_.handlePasskey(passKey);
  }

  bool onSecurityRequest() override {
    service_.pairingRequested_ = true;
    service_.publish(EventType::BlePairingStarted, "Security request");
    return true;
  }

  void onAuthenticationComplete(esp_ble_auth_cmpl_t result) override {
    service_.handleAuthenticationComplete(result.success);
  }

  bool onConfirmPIN(uint32_t pin) override {
    service_.handlePasskey(pin);
    return true;
  }

 private:
  BluetoothService& service_;
};

bool BluetoothService::begin() {
  prefs_.begin("iris_ble", false);
  enabled_ = prefs_.getBool("enabled", false);
  autoReconnect_ = prefs_.getBool("autorec", true);
  deviceName_ = prefs_.getString("name", "");
  if (deviceName_.isEmpty()) deviceName_ = String("Iris-") + deviceSuffix();
  if (!enabled_) return true;
  return initializeBle();
}

void BluetoothService::update(uint32_t nowMs) {
  if (releaseAtMs_ != 0 && static_cast<int32_t>(nowMs - releaseAtMs_) >= 0) {
    releaseConsumerUsage();
  }
  if (pendingAdvertiseRestart_ &&
      static_cast<int32_t>(nowMs - restartAdvertisingAtMs_) >= 0) {
    pendingAdvertiseRestart_ = false;
    startAdvertising();
  }
}

void BluetoothService::setEnabled(bool enabled) {
  if (enabled_ == enabled) return;
  enabled_ = enabled;
  prefs_.putBool("enabled", enabled_);
  if (!enabled_) {
    stopAdvertising();
    connected_ = false;
    activeDevice_ = "";
    publish(EventType::BleDisabled, "BLE disabled");
    return;
  }
  initializeBle();
  startAdvertising();
  publish(EventType::BleEnabled, "BLE enabled");
}

void BluetoothService::setAutoReconnect(bool enabled) {
  autoReconnect_ = enabled;
  prefs_.putBool("autorec", autoReconnect_);
}

uint32_t BluetoothService::bondedDeviceCount() const {
  if (!initialized_) return 0;
  return static_cast<uint32_t>(esp_ble_get_bond_device_num());
}

String BluetoothService::statusText() const {
  if (!enabled_) return "Disabled";
  if (!initialized_) return "Not initialized";
  if (connected_) return "BLE HID connected";
  if (advertising_) return "Advertising";
  if (pairingFailed_) return "Pairing failed";
  return "Disconnected";
}

String BluetoothService::json() const {
  String json;
  json.reserve(420);
  json += F("{\"enabled\":");
  json += enabled_ ? F("true") : F("false");
  json += F(",\"technology\":\"Bluetooth Low Energy (BLE)\",\"classicSupported\":false");
  json += F(",\"initialized\":");
  json += initialized_ ? F("true") : F("false");
  json += F(",\"advertising\":");
  json += advertising_ ? F("true") : F("false");
  json += F(",\"connected\":");
  json += connected_ ? F("true") : F("false");
  json += F(",\"autoReconnect\":");
  json += autoReconnect_ ? F("true") : F("false");
  json += F(",\"deviceName\":\"");
  json += escapeJson(deviceName_);
  json += F("\",\"activeDevice\":\"");
  json += escapeJson(activeDevice_);
  json += F("\",\"status\":\"");
  json += escapeJson(statusText());
  json += F("\",\"bondedDevices\":");
  json += String(bondedDeviceCount());
  json += F(",\"profile\":\"BLE HID Consumer Control\"}");
  return json;
}

bool BluetoothService::startAdvertising() {
  if (!enabled_ || !initializeBle() || !advertisingHandle_) return false;
  if (connected_) return true;
  advertisingHandle_->start();
  advertising_ = true;
  pairingFailed_ = false;
  Serial.printf("[BLE] Advertising as %s\n", deviceName_.c_str());
  publish(EventType::BlePairingStarted, deviceName_.c_str());
  return true;
}

void BluetoothService::stopAdvertising() {
  if (!initialized_ || !advertisingHandle_ || !advertising_) return;
  advertisingHandle_->stop();
  advertising_ = false;
}

void BluetoothService::disconnect() {
  if (!server_ || !connected_) return;
  server_->disconnect(connectionId_);
}

bool BluetoothService::sendMediaCommand(BleMediaCommand command) {
  if (!connected_ || !input_) {
    Serial.printf("[BLE] Media command ignored while disconnected: %s\n", commandName(command));
    return false;
  }
  sendConsumerUsage(usageFor(command));
  releaseAtMs_ = millis() + kKeyReleaseDelayMs;
  lastCommandMs_ = millis();
  Serial.printf("[BLE] Media command: %s\n", commandName(command));
  return true;
}

bool BluetoothService::initializeBle() {
  if (initialized_) return true;
  if (!enabled_) return false;

  Serial.println("[BLE] Initializing BLE stack");
  BLEDevice::init(deviceName_.c_str());
  configureSecurity();

  server_ = BLEDevice::createServer();
  if (!server_) return false;
  serverCallbacks_ = new ServerCallbacks(*this);
  server_->setCallbacks(serverCallbacks_);

  hid_ = new BLEHIDDevice(server_);
  input_ = hid_->inputReport(kReportIdConsumer);
  input_->addDescriptor(new BLE2902());
  hid_->manufacturer("Iris");
  hid_->pnp(0x02, 0x1209, 0x0001, 0x0100);
  hid_->hidInfo(0x00, 0x01);
  hid_->reportMap(kConsumerReportMap, sizeof(kConsumerReportMap));
  hid_->startServices();

  advertisingHandle_ = BLEDevice::getAdvertising();
  advertisingHandle_->setAppearance(ESP_BLE_APPEARANCE_GENERIC_HID);
  advertisingHandle_->addServiceUUID(hid_->hidService()->getUUID());
  advertisingHandle_->setScanResponse(false);
  advertisingHandle_->setMinPreferred(0x06);
  advertisingHandle_->setMaxPreferred(0x12);

  initialized_ = true;
  Serial.println("[BLE] HID service registered");
  return true;
}

void BluetoothService::configureSecurity() {
  securityCallbacks_ = new SecurityCallbacks(*this);
  BLEDevice::setSecurityCallbacks(securityCallbacks_);
  BLESecurity* security = new BLESecurity();
  security->setAuthenticationMode(ESP_LE_AUTH_BOND);
  security->setCapability(ESP_IO_CAP_OUT);
  security->setKeySize();
  security->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
  security->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
}

void BluetoothService::sendConsumerUsage(uint16_t usage) {
  if (!input_) return;
  uint8_t report[2] = {
      static_cast<uint8_t>(usage & 0xFF),
      static_cast<uint8_t>((usage >> 8) & 0xFF),
  };
  input_->setValue(report, sizeof(report));
  input_->notify();
}

void BluetoothService::releaseConsumerUsage() {
  if (!input_) return;
  uint8_t report[2] = {0, 0};
  input_->setValue(report, sizeof(report));
  input_->notify();
  releaseAtMs_ = 0;
}

void BluetoothService::publish(EventType type, const char* text, int32_t value) {
  if (events_) events_->publish(type, "BluetoothService", text, value);
}

void BluetoothService::handleConnected(const String& address, uint16_t connectionId) {
  connected_ = true;
  advertising_ = false;
  pairingFailed_ = false;
  pairingRequested_ = false;
  connectionId_ = connectionId;
  if (!address.isEmpty()) activeDevice_ = address;
  Serial.printf("[BLE] HID host connected %s\n", activeDevice_.c_str());
  publish(EventType::BleConnected, activeDevice_.c_str());
}

void BluetoothService::handleDisconnected() {
  if (!connected_) return;
  connected_ = false;
  connectionId_ = 0;
  Serial.println("[BLE] HID host disconnected");
  publish(EventType::BleDisconnected, activeDevice_.c_str());
  if (enabled_ && autoReconnect_) {
    pendingAdvertiseRestart_ = true;
    restartAdvertisingAtMs_ = millis() + kAdvertiseRestartDelayMs;
  }
}

void BluetoothService::handlePasskey(uint32_t passkey) {
  passkey_ = passkey;
  pairingRequested_ = true;
  Serial.printf("[BLE] Pairing passkey %06lu\n", static_cast<unsigned long>(passkey_));
}

void BluetoothService::handleAuthenticationComplete(bool success) {
  pairingRequested_ = false;
  pairingFailed_ = !success;
  publish(success ? EventType::BleDevicePaired : EventType::BlePairingFailed,
          success ? "Pairing complete" : "Pairing failed");
  Serial.println(success ? "[BLE] Device bonded" : "[BLE] Pairing failed");
}

uint16_t BluetoothService::usageFor(BleMediaCommand command) const {
  switch (command) {
    case BleMediaCommand::NextTrack: return kUsageScanNext;
    case BleMediaCommand::PreviousTrack: return kUsageScanPrevious;
    case BleMediaCommand::VolumeUp: return kUsageVolumeUp;
    case BleMediaCommand::VolumeDown: return kUsageVolumeDown;
    case BleMediaCommand::Mute: return kUsageMute;
    default: return kUsagePlayPause;
  }
}

const char* BluetoothService::commandName(BleMediaCommand command) const {
  switch (command) {
    case BleMediaCommand::NextTrack: return "Next";
    case BleMediaCommand::PreviousTrack: return "Previous";
    case BleMediaCommand::VolumeUp: return "Volume up";
    case BleMediaCommand::VolumeDown: return "Volume down";
    case BleMediaCommand::Mute: return "Mute";
    default: return "Play/Pause";
  }
}

String BluetoothService::escapeJson(const String& value) {
  String escaped;
  escaped.reserve(value.length() + 8);
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value[i];
    if (c == '\\' || c == '"') {
      escaped += '\\';
      escaped += c;
    } else {
      escaped += c;
    }
  }
  return escaped;
}

}  // namespace iris
