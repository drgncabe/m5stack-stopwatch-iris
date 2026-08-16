#include "iris/services/WifiService.h"

#include <ESP.h>

#include "iris/AppConfig.h"

namespace iris {

WifiService::WifiService() : server_(80) {}

void WifiService::begin(bool enabled) {
  prefs_.begin("iris_wifi", false);
  savedSsid_ = prefs_.getString("ssid", "");
  savedPassword_ = prefs_.getString("password", "");
  enabled_ = enabled;

  if (enabled_) {
    connectSaved();
  } else {
    WiFi.mode(WIFI_OFF);
  }
}

void WifiService::update(uint32_t nowMs) {
  if (portalRunning_) {
    server_.handleClient();
    return;
  }

  if (!enabled_ || savedSsid_.isEmpty() || isConnected()) return;

  if (nowMs - lastConnectAttemptMs_ >= config::kWifiReconnectMs) {
    connectSaved();
  }
}

void WifiService::setEnabled(bool enabled) {
  enabled_ = enabled;

  if (!enabled_) {
    stopProvisioning();
    WiFi.disconnect(true, false);
    WiFi.mode(WIFI_OFF);
    return;
  }

  connectSaved();
}

String WifiService::ssid() const {
  if (isConnected()) return WiFi.SSID();
  return savedSsid_;
}

String WifiService::ipAddress() const {
  if (isConnected()) return WiFi.localIP().toString();
  if (portalRunning_) return WiFi.softAPIP().toString();
  return "--";
}

String WifiService::statusText() const {
  if (!enabled_) return "Disabled";
  if (portalRunning_) return "Setup portal active";
  if (isConnected()) return "Connected";
  if (savedSsid_.isEmpty()) return "Not configured";
  return "Connecting...";
}

void WifiService::connectSaved() {
  if (!enabled_ || savedSsid_.isEmpty()) return;

  if (portalRunning_) {
    server_.stop();
    WiFi.softAPdisconnect(true);
    portalRunning_ = false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSsid_.c_str(), savedPassword_.c_str());
  lastConnectAttemptMs_ = millis();
  Serial.printf("Iris WiFi: connecting to %s\n", savedSsid_.c_str());
}

void WifiService::startProvisioning() {
  if (!enabled_) enabled_ = true;
  if (portalRunning_) return;

  const uint64_t chipId = ESP.getEfuseMac();
  char suffix[5];
  snprintf(suffix, sizeof(suffix), "%04X", static_cast<uint16_t>(chipId & 0xFFFF));
  portalSsid_ = String("Iris-Setup-") + suffix;

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(portalSsid_.c_str());
  configurePortalRoutes();
  server_.begin();
  portalRunning_ = true;

  Serial.printf("Iris WiFi setup: connect to %s and open http://%s\n",
                portalSsid_.c_str(), WiFi.softAPIP().toString().c_str());
}

void WifiService::stopProvisioning() {
  if (!portalRunning_) return;

  server_.stop();
  WiFi.softAPdisconnect(true);
  portalRunning_ = false;

  if (enabled_) {
    connectSaved();
  } else {
    WiFi.mode(WIFI_OFF);
  }
}

void WifiService::configurePortalRoutes() {
  server_.on("/", HTTP_GET, [this]() { handlePortalIndex(); });
  server_.on("/save", HTTP_POST, [this]() { handlePortalSave(); });
  server_.onNotFound([this]() {
    server_.sendHeader("Location", "/", true);
    server_.send(302, "text/plain", "");
  });
}

void WifiService::handlePortalIndex() {
  String html;
  html.reserve(5000);
  html += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>Iris WiFi Setup</title><style>body{font-family:system-ui;background:#111;color:#eee;max-width:560px;margin:40px auto;padding:0 18px}label{display:block;margin-top:18px}select,input,button{box-sizing:border-box;width:100%;font-size:17px;padding:12px;margin-top:6px;border-radius:8px;border:1px solid #555;background:#222;color:#fff}button{background:#fff;color:#111;font-weight:700}</style></head><body>");
  html += F("<h1>Iris WiFi Setup</h1><p>Select a network and enter its password. Credentials are stored only on the StopWatch.</p><form method='post' action='/save'><label>Network</label><select name='ssid' required>");

  const int count = WiFi.scanNetworks();
  if (count <= 0) {
    html += F("<option value=''>No networks found</option>");
  } else {
    for (int i = 0; i < count; ++i) {
      const String network = WiFi.SSID(i);
      html += F("<option value='");
      html += escapeHtml(network);
      html += F("'>");
      html += escapeHtml(network);
      html += F(" (");
      html += String(WiFi.RSSI(i));
      html += F(" dBm)</option>");
    }
  }
  WiFi.scanDelete();

  html += F("</select><label>Password</label><input name='password' type='password' autocomplete='current-password'><button type='submit'>Save & Connect</button></form></body></html>");
  server_.send(200, "text/html", html);
}

void WifiService::handlePortalSave() {
  const String ssidValue = server_.arg("ssid");
  const String passwordValue = server_.arg("password");

  if (ssidValue.isEmpty()) {
    server_.send(400, "text/plain", "SSID is required.");
    return;
  }

  savedSsid_ = ssidValue;
  savedPassword_ = passwordValue;
  prefs_.putString("ssid", savedSsid_);
  prefs_.putString("password", savedPassword_);

  server_.send(200, "text/html",
               "<!doctype html><html><body style='font-family:system-ui'><h2>Saved</h2><p>Iris is connecting. You can close this page.</p></body></html>");

  delay(250);
  stopProvisioning();
}

String WifiService::escapeHtml(const String& value) {
  String escaped;
  escaped.reserve(value.length() + 8);
  for (size_t i = 0; i < value.length(); ++i) {
    switch (value[i]) {
      case '&': escaped += F("&amp;"); break;
      case '<': escaped += F("&lt;"); break;
      case '>': escaped += F("&gt;"); break;
      case '\"': escaped += F("&quot;"); break;
      case '\'': escaped += F("&#39;"); break;
      default: escaped += value[i]; break;
    }
  }
  return escaped;
}

}  // namespace iris
