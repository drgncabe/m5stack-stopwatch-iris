#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

namespace iris {

class WifiService {
 public:
  WifiService();

  void begin(bool enabled);
  void update(uint32_t nowMs);
  void setEnabled(bool enabled);

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
  void configurePortalRoutes();
  void handlePortalIndex();
  void handlePortalSave();
  static String escapeHtml(const String& value);

  Preferences prefs_;
  WebServer server_;
  String savedSsid_;
  String savedPassword_;
  String portalSsid_;
  bool enabled_ = true;
  bool portalRunning_ = false;
  uint32_t lastConnectAttemptMs_ = 0;
};

}  // namespace iris
