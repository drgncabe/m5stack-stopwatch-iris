#include "iris/services/WifiService.h"

#include "iris/AppConfig.h"

namespace iris {

WifiService::WifiService() : server_(80) {}

void WifiService::begin(bool enabled) {
  prefs_.begin("iris_wifi", false);
  savedSsid_ = prefs_.getString("ssid", "");
  savedPassword_ = prefs_.getString("password", "");
  enabled_ = enabled;
  configurePortalRoutes();

  if (enabled_) {
    connectSaved();
  } else {
    WiFi.mode(WIFI_OFF);
  }
}

void WifiService::update(uint32_t nowMs) {
  if (serverRunning_) {
    server_.handleClient();
  }

  if (portalRunning_) return;
  if (!enabled_ || savedSsid_.isEmpty() || isConnected()) return;

  if (nowMs - lastConnectAttemptMs_ >= config::kWifiReconnectMs) {
    connectSaved();
  }
}

void WifiService::setEnabled(bool enabled) {
  enabled_ = enabled;

  if (!enabled_) {
    stopProvisioning();
    if (serverRunning_) {
      server_.stop();
      serverRunning_ = false;
    }
    WiFi.disconnect(true, false);
    WiFi.mode(WIFI_OFF);
    return;
  }

  connectSaved();
}

void WifiService::setControlCallbacks(void* context,
                                      ControlCommandHandler commandHandler,
                                      ControlSnapshotHandler snapshotHandler) {
  controlContext_ = context;
  commandHandler_ = commandHandler;
  snapshotHandler_ = snapshotHandler;
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
  ensureServer();
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
  ensureServer();
  portalRunning_ = true;

  Serial.printf("Iris WiFi setup: connect to %s and open http://%s\n",
                portalSsid_.c_str(), WiFi.softAPIP().toString().c_str());
}

void WifiService::stopProvisioning() {
  if (!portalRunning_) return;

  WiFi.softAPdisconnect(true);
  portalRunning_ = false;

  if (enabled_) {
    connectSaved();
  } else {
    WiFi.mode(WIFI_OFF);
  }
}

void WifiService::configurePortalRoutes() {
  if (routesConfigured_) return;

  server_.on("/", HTTP_GET, [this]() { handleControlPanel(); });
  server_.on("/control", HTTP_GET, [this]() { handleControlCommand(); });
  server_.on("/display.txt", HTTP_GET, [this]() { handleDisplaySnapshot(); });
  server_.on("/setup", HTTP_GET, [this]() { handleWifiSetup(); });
  server_.on("/save", HTTP_POST, [this]() { handlePortalSave(); });
  server_.onNotFound([this]() {
    server_.sendHeader("Location", "/", true);
    server_.send(302, "text/plain", "");
  });
  routesConfigured_ = true;
}

void WifiService::ensureServer() {
  configurePortalRoutes();
  if (serverRunning_) return;

  server_.begin();
  serverRunning_ = true;
}

void WifiService::handleControlPanel() {
  String html;
  html.reserve(5000);
  html += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>Iris Control</title><style>body{font-family:system-ui;background:#111;color:#eee;max-width:720px;margin:32px auto;padding:0 18px}.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}a,button{display:block;text-align:center;text-decoration:none;box-sizing:border-box;width:100%;font-size:16px;padding:13px;margin:0;border-radius:8px;border:1px solid #555;background:#222;color:#fff}section{margin:24px 0;padding-top:8px;border-top:1px solid #333}pre{white-space:pre-wrap;background:#050505;border:1px solid #333;border-radius:8px;padding:12px}</style></head><body>");
  html += F("<h1>Iris Control</h1>");
  html += F("<section><h2>Display</h2><pre>");
  html += snapshotHandler_ ? escapeHtml(snapshotHandler_(controlContext_)) : String("No snapshot available");
  html += F("</pre></section><section><h2>Controls</h2><div class='grid'>");
  html += F("<a href='/control?cmd=watch'>Watch</a><a href='/control?cmd=settings'>Settings</a>");
  html += F("<a href='/control?cmd=btn_a'>BtnA</a><a href='/control?cmd=btn_b'>BtnB</a>");
  html += F("<a href='/control?cmd=vol_down'>Volume -</a><a href='/control?cmd=vol_up'>Volume +</a>");
  html += F("<a href='/control?cmd=bg_next'>Next background</a><a href='/control?cmd=wifi_toggle'>Toggle WiFi</a>");
  html += F("</div></section><section><h2>WiFi</h2><div class='grid'>");
  html += F("<a href='/control?cmd=wifi_setup'>Start setup AP</a><a href='/setup'>Choose network</a>");
  html += F("</div></section><p><a href='/display.txt'>Plain display snapshot</a></p></body></html>");
  server_.send(200, "text/html", html);
}

void WifiService::handleControlCommand() {
  const String command = server_.arg("cmd");
  if (commandHandler_ && !command.isEmpty()) {
    commandHandler_(controlContext_, command);
  }

  server_.sendHeader("Location", "/", true);
  server_.send(302, "text/plain", "");
}

void WifiService::handleDisplaySnapshot() {
  const String snapshot = snapshotHandler_ ? snapshotHandler_(controlContext_) : String("No snapshot available");
  server_.send(200, "text/plain", snapshot);
}

void WifiService::handleWifiSetup() {
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
      case '"': escaped += F("&quot;"); break;
      case '\'': escaped += F("&#39;"); break;
      default: escaped += value[i]; break;
    }
  }
  return escaped;
}

}  // namespace iris
