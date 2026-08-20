#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

namespace iris {

using ControlCommandHandler = void (*)(void* context, const String& command);
using ControlSnapshotHandler = String (*)(void* context);

class WifiService {
 public:
  WifiService();

  void begin(bool enabled);
  void update(uint32_t nowMs);
  void setEnabled(bool enabled);
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

 private:
  void connectSaved();
  void ensureServer();
  void configurePortalRoutes();
  void handleControlPanel();
  void handleControlCommand();
  void handleDisplaySnapshot();
  void handleWifiSetup();
  void handlePortalSave();
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
  uint32_t lastConnectAttemptMs_ = 0;
  uint32_t portalStartedMs_ = 0;
  void* controlContext_ = nullptr;
  ControlCommandHandler commandHandler_ = nullptr;
  ControlSnapshotHandler snapshotHandler_ = nullptr;
};

}  // namespace iris
