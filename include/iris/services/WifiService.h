#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

#include "iris/services/BadgeService.h"
#include "iris/services/BluetoothService.h"

namespace iris {

using ControlCommandHandler = void (*)(void* context, const String& command);
using ControlSnapshotHandler = String (*)(void* context);

class WifiService {
 public:
  WifiService();

  void begin(bool enabled);
  void update(uint32_t nowMs);
  void setEnabled(bool enabled);
  void setBadgeService(BadgeService* badge);
  void setBluetoothService(BluetoothService* bluetooth);
  void setControlCallbacks(void* context,
                           ControlCommandHandler commandHandler,
                           ControlSnapshotHandler snapshotHandler);

  bool isEnabled() const { return enabled_; }
  bool isConnected() const { return WiFi.status() == WL_CONNECTED; }
  bool isProvisioning() const { return portalRunning_; }
  bool hasSavedNetwork() const { return savedSsid_.length() > 0; }

  String ssid() const;
  String ipAddress() const;
  String statusText() const;
  String portalSsid() const { return portalSsid_; }

  void startProvisioning();
  void stopProvisioning();
  void shutdownForBootloader();

 private:
  void connectSaved();
  void ensureServer();
  void configurePortalRoutes();
  void handleControlPanel();
  void handleControlCommand();
  void handleApiSettings();
  void handleApiDisplaySettings();
  void handleApiPowerSettings();
  void handleApiTouchSettings();
  void handleApiSoundSettings();
  void handleApiThemeSettings();
  void handleApiTimeSettings();
  void handleApiDeviceInfo();
  void handleApiApps();
  void handleApiBadge();
  void handleApiBadgeDelete();
  void handleApiBluetooth();
  void handleBadgeAsset();
  void handleBadgeUploadDone();
  void handleBadgeUpload();
  void handleApiCommand();
  void handleApiWifiStatus();
  void handleApiWifiNetworks();
  void handleApiWifiConnect();
  void handleApiWifiDisconnect();
  void handleApiWifiReconnect();
  void handleApiWifiForget();
  void handleDisplaySnapshot();
  void handleWifiSetup();
  void handlePortalSave();
  void appendPageShellStart(String& html, const String& page, const String& snapshot);
  void appendPageShellEnd(String& html);
  void appendWatchPreview(String& html, const String& snapshot);
  void appendNavigation(String& html, const String& page);
  void appendSettingsCard(String& html, const char* href, const char* title,
                          const char* detail);
  void appendAppRegistry(String& html, const String& registry);
  void appendBadgePage(String& html);
  void appendBluetoothPage(String& html, const String& snapshot);
  void appendRangeControl(String& html, const char* label, const char* command,
                          int value, int minValue, int maxValue, int step,
                          const char* suffix);
  void appendToggleControl(String& html, const char* label, const char* command,
                           bool enabled);
  void appendAction(String& html, const char* label, const char* command,
                    const char* className = "");
  String snapshotValue(const String& snapshot, const char* key) const;
  int snapshotInt(const String& snapshot, const char* key, int fallback) const;
  bool snapshotOn(const String& snapshot, const char* key) const;
  bool dispatchControlCommand(const String& command);
  void disconnectStation();
  void forgetSavedNetwork();
  String apiArg(const char* name);
  bool apiIntArg(const char* name, int* value);
  bool apiBoolArg(const char* name, bool* value);
  bool apiStringArg(const char* name, String* value);
  void sendApiError(int code, const char* message);
  void sendApiOk(const char* message);
  static String escapeJson(const String& value);
  static String escapeHtml(const String& value);

  Preferences prefs_;
  WebServer server_;
  String savedSsid_;
  String savedPassword_;
  String portalSsid_;
  bool enabled_ = true;
  bool portalRunning_ = false;
  bool serverRunning_ = false;
  bool routesConfigured_ = false;
  bool badgeUploadOk_ = false;
  uint32_t lastConnectAttemptMs_ = 0;
  uint32_t portalStartedMs_ = 0;
  void* controlContext_ = nullptr;
  ControlCommandHandler commandHandler_ = nullptr;
  ControlSnapshotHandler snapshotHandler_ = nullptr;
  BadgeService* badge_ = nullptr;
  BluetoothService* bluetooth_ = nullptr;
};

}  // namespace iris
