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
    WiFi.setSleep(false);
    WiFi.mode(WIFI_OFF);
  }
}

void WifiService::update(uint32_t nowMs) {
  if (serverRunning_) {
    server_.handleClient();
  }

  if (portalRunning_) {
    if (nowMs - portalStartedMs_ >= config::kWifiPortalTimeoutMs) {
      stopProvisioning();
    }
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
    if (serverRunning_) {
      server_.stop();
      serverRunning_ = false;
    }
    WiFi.disconnect(true, false);
    WiFi.setSleep(false);
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
  WiFi.setSleep(true);
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
  WiFi.setSleep(false);
  WiFi.softAP(portalSsid_.c_str());
  ensureServer();
  portalRunning_ = true;
  portalStartedMs_ = millis();

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
  server_.on("/api/settings", HTTP_GET, [this]() { handleApiSettings(); });
  server_.on("/api/settings/display", HTTP_GET, [this]() { handleApiDisplaySettings(); });
  server_.on("/api/settings/display", HTTP_PUT, [this]() { handleApiDisplaySettings(); });
  server_.on("/api/settings/power", HTTP_GET, [this]() { handleApiPowerSettings(); });
  server_.on("/api/settings/power", HTTP_PUT, [this]() { handleApiPowerSettings(); });
  server_.on("/api/settings/touch", HTTP_GET, [this]() { handleApiTouchSettings(); });
  server_.on("/api/settings/touch", HTTP_PUT, [this]() { handleApiTouchSettings(); });
  server_.on("/api/settings/sound", HTTP_GET, [this]() { handleApiSoundSettings(); });
  server_.on("/api/settings/sound", HTTP_PUT, [this]() { handleApiSoundSettings(); });
  server_.on("/api/settings/audio", HTTP_GET, [this]() { handleApiSoundSettings(); });
  server_.on("/api/settings/audio", HTTP_PUT, [this]() { handleApiSoundSettings(); });
  server_.on("/api/settings/theme", HTTP_GET, [this]() { handleApiThemeSettings(); });
  server_.on("/api/settings/theme", HTTP_PUT, [this]() { handleApiThemeSettings(); });
  server_.on("/api/command", HTTP_POST, [this]() { handleApiCommand(); });
  server_.on("/api/wifi/status", HTTP_GET, [this]() { handleApiWifiStatus(); });
  server_.on("/api/wifi/status", HTTP_POST, [this]() { handleApiWifiStatus(); });
  server_.on("/api/wifi/setup", HTTP_POST, [this]() { handleApiCommand(); });
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
  const String snapshot = snapshotHandler_ ? snapshotHandler_(controlContext_) : String("No snapshot available");
  String page = server_.arg("page");
  if (page.isEmpty()) page = "dashboard";

  String html;
  html.reserve(14200);
  appendPageShellStart(html, page, snapshot);

  if (page == "display") {
    html += F("<section><h2>Display</h2>");
    appendRangeControl(html, "Brightness", "brightness_set", snapshotInt(snapshot, "Brightness", 96), 16, 255, 1, "/255");
    appendRangeControl(html, "Dim brightness", "dim_brightness_set", snapshotInt(snapshot, "Dim brightness", 18), 1, 96, 1, "/255");
    appendRangeControl(html, "Dim timeout", "dim_set", snapshotInt(snapshot, "Dim timeout", 20), 5, 120, 1, " sec");
    appendRangeControl(html, "Sleep timeout", "sleep_set", snapshotInt(snapshot, "Sleep timeout", 90), 0, 600, 5, " sec");
    appendToggleControl(html, "Low-power watch face", "low_face_toggle", snapshotOn(snapshot, "Low-power face"));
    appendToggleControl(html, "Automatic rotation", "auto_rotate_toggle", snapshotValue(snapshot, "Auto rotate") != "Off");
    html += F("</section>");
  } else if (page == "touch") {
    html += F("<section><h2>Touch</h2>");
    appendRangeControl(html, "Touch delay", "touch_set", snapshotInt(snapshot, "Touch delay", 150), 50, 500, 5, " ms");
    html += F("<p class='hint'>Menu screens still enforce a slightly longer minimum hold so scrolling does not accidentally select an item.</p></section>");
  } else if (page == "sound") {
    html += F("<section><h2>Sound</h2>");
    appendRangeControl(html, "Master volume", "volume_set", snapshotInt(snapshot, "Volume", 38), 0, 100, 1, "%");
    appendAction(html, "Volume -", "vol_down");
    appendAction(html, "Volume +", "vol_up");
    html += F("</section>");
  } else if (page == "wifi") {
    html += F("<section><h2>WiFi</h2><div class='facts'>");
    html += F("<p><b>Status</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "WiFi"));
    html += F("</span></p><p><b>Network</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "SSID"));
    html += F("</span></p><p><b>IP Address</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "IP"));
    html += F("</span></p></div>");
    appendToggleControl(html, "WiFi enabled", "wifi_toggle", isEnabled());
    appendToggleControl(html, "WiFi on demand", "wifi_demand_toggle", snapshotOn(snapshot, "WiFi on demand"));
    appendAction(html, "Start setup AP", "wifi_setup", "warn");
    html += F("<a class='button' href='/setup'>Choose network</a></section>");
  } else if (page == "theme") {
    html += F("<section><h2>Theme</h2><div class='grid three'>");
    appendAction(html, "Black", "theme_0");
    appendAction(html, "Midnight", "theme_1");
    appendAction(html, "Forest", "theme_2");
    appendAction(html, "Plum", "theme_3");
    appendAction(html, "Steel", "theme_4");
    appendAction(html, "Next theme", "bg_next");
    html += F("</div><h3>Widgets</h3><div class='grid'>");
    appendAction(html, "Toggle battery", "widget_battery_toggle");
    appendAction(html, "Toggle date", "widget_date_toggle");
    appendAction(html, "Toggle seconds", "widget_seconds_toggle");
    appendAction(html, "Toggle WiFi", "widget_wifi_toggle");
    appendAction(html, "Next complication", "complication_next");
    html += F("</div></section>");
  } else if (page == "power") {
    html += F("<section><h2>Power</h2>");
    appendRangeControl(html, "Brightness", "brightness_set", snapshotInt(snapshot, "Brightness", 96), 16, 255, 1, "/255");
    appendRangeControl(html, "Dim brightness", "dim_brightness_set", snapshotInt(snapshot, "Dim brightness", 18), 1, 96, 1, "/255");
    appendRangeControl(html, "Dim timeout", "dim_set", snapshotInt(snapshot, "Dim timeout", 20), 5, 120, 1, " sec");
    appendRangeControl(html, "Screen-off timeout", "sleep_set", snapshotInt(snapshot, "Sleep timeout", 90), 0, 600, 5, " sec");
    html += F("<div class='control'><label>Power profile<span>");
    html += escapeHtml(snapshotValue(snapshot, "Power profile"));
    html += F("</span></label>");
    appendAction(html, "Next profile", "power_profile_next");
    html += F("<p class='hint'>Runtime favors cooler operation, Balanced is the default, and Performance keeps animation-heavy apps at full speed.</p></div>");
    appendToggleControl(html, "Low-power watch face", "low_face_toggle", snapshotOn(snapshot, "Low-power face"));
    appendToggleControl(html, "WiFi on demand", "wifi_demand_toggle", snapshotOn(snapshot, "WiFi on demand"));
    appendToggleControl(html, "Indicator light", "indicator_toggle", snapshotOn(snapshot, "Indicator light"));
    html += F("</section>");
  } else if (page == "device") {
    html += F("<section><h2>Device</h2><div class='facts'>");
    html += F("<p><b>Current screen</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Screen"));
    html += F("</span></p><p><b>Display power</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Display power"));
    html += F("</span></p><p><b>Power profile</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Power profile"));
    html += F("</span></p><p><b>CPU</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "CPU"));
    html += F("</span></p><p><b>Battery</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Battery"));
    html += F("</span></p><p><b>Theme</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Theme"));
    html += F("</span></p><p><b>Face layout</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Face layout"));
    html += F("</span></p></div><h3>Raw snapshot</h3><pre>");
    html += escapeHtml(snapshot);
    html += F("</pre><a class='button' href='/display.txt'>Plain text snapshot</a><a class='button' href='/api/settings'>JSON settings API</a><a class='button' href='/api/settings/display'>Display API</a><a class='button' href='/api/settings/touch'>Touch API</a><a class='button' href='/api/settings/sound'>Sound API</a><a class='button' href='/api/settings/theme'>Theme API</a><a class='button' href='/api/settings/power'>Power API</a><a class='button' href='/api/wifi/status'>WiFi API</a></section>");
  } else if (page == "development") {
    html += F("<section><h2>Development</h2><div class='grid'>");
    appendAction(html, "Watch screen", "watch");
    appendAction(html, "Settings screen", "settings");
    appendAction(html, "BtnA", "btn_a");
    appendAction(html, "BtnB", "btn_b");
    html += F("</div><p class='hint'>Remote screen control lives here now so normal settings pages stay focused on configuration.</p></section>");
  } else {
    html += F("<section><h2>Dashboard</h2><div class='facts'>");
    html += F("<p><b>Battery</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Battery"));
    html += F("</span></p><p><b>WiFi</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "WiFi"));
    html += F("</span></p><p><b>Network</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "SSID"));
    html += F("</span></p><p><b>Theme</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Theme"));
    html += F("</span></p><p><b>Screen</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Screen"));
    html += F("</span></p></div><div class='grid'>");
    html += F("<a class='button' href='/?page=display'>Display</a><a class='button' href='/?page=wifi'>WiFi</a>");
    html += F("<a class='button' href='/?page=theme'>Theme</a><a class='button' href='/?page=power'>Power</a>");
    html += F("</div></section>");
  }

  appendPageShellEnd(html);
  server_.send(200, "text/html", html);
}

void WifiService::handleControlCommand() {
  String command = server_.arg("cmd");
  if (server_.hasArg("value")) {
    command += ":";
    command += server_.arg("value");
  }
  dispatchControlCommand(command);

  String destination = "/";
  if (server_.hasArg("return")) {
    destination = server_.arg("return");
  } else if (server_.hasArg("page")) {
    destination = String("/?page=") + server_.arg("page");
  }
  server_.sendHeader("Location", destination, true);
  server_.send(302, "text/plain", "");
}

void WifiService::handleApiSettings() {
  const String snapshot = snapshotHandler_ ? snapshotHandler_(controlContext_) : String("");
  String json;
  json.reserve(760);
  json += F("{\"screen\":\"");
  json += escapeJson(snapshotValue(snapshot, "Screen"));
  json += F("\",\"displayPower\":\"");
  json += escapeJson(snapshotValue(snapshot, "Display power"));
  json += F("\",\"powerProfile\":\"");
  json += escapeJson(snapshotValue(snapshot, "Power profile"));
  json += F("\",\"cpu\":\"");
  json += escapeJson(snapshotValue(snapshot, "CPU"));
  json += F("\",\"battery\":\"");
  json += escapeJson(snapshotValue(snapshot, "Battery"));
  json += F("\",\"wifi\":\"");
  json += escapeJson(snapshotValue(snapshot, "WiFi"));
  json += F("\",\"ssid\":\"");
  json += escapeJson(snapshotValue(snapshot, "SSID"));
  json += F("\",\"ip\":\"");
  json += escapeJson(snapshotValue(snapshot, "IP"));
  json += F("\",\"volumePercent\":");
  json += String(snapshotInt(snapshot, "Volume", 0));
  json += F(",\"theme\":\"");
  json += escapeJson(snapshotValue(snapshot, "Theme"));
  json += F("\",\"layout\":\"");
  json += escapeJson(snapshotValue(snapshot, "Face layout"));
  json += F("\",\"brightness\":");
  json += String(snapshotInt(snapshot, "Brightness", 0));
  json += F(",\"dimBrightness\":");
  json += String(snapshotInt(snapshot, "Dim brightness", 0));
  json += F(",\"dimTimeoutSeconds\":");
  json += String(snapshotInt(snapshot, "Dim timeout", 0));
  json += F(",\"sleepTimeoutSeconds\":");
  json += String(snapshotInt(snapshot, "Sleep timeout", 0));
  json += F(",\"touchDelayMs\":");
  json += String(snapshotInt(snapshot, "Touch delay", 0));
  json += F("}");
  server_.send(200, "application/json", json);
}

void WifiService::handleApiDisplaySettings() {
  if (server_.method() == HTTP_PUT) {
    int intValue = 0;
    bool boolValue = false;
    const String snapshot = snapshotHandler_ ? snapshotHandler_(controlContext_) : String("");
    if (apiIntArg("brightness", &intValue)) {
      dispatchControlCommand(String("brightness_set:") + intValue);
    }
    if (apiIntArg("dimBrightness", &intValue)) {
      dispatchControlCommand(String("dim_brightness_set:") + intValue);
    }
    if (apiIntArg("dimTimeoutSeconds", &intValue)) {
      dispatchControlCommand(String("dim_set:") + intValue);
    }
    if (apiIntArg("sleepTimeoutSeconds", &intValue)) {
      dispatchControlCommand(String("sleep_set:") + intValue);
    }
    if (apiBoolArg("lowPowerFace", &boolValue) &&
        boolValue != snapshotOn(snapshot, "Low-power face")) {
      dispatchControlCommand("low_face_toggle");
    }
    if (apiBoolArg("autoRotate", &boolValue) &&
        boolValue != (snapshotValue(snapshot, "Auto rotate") != "Off")) {
      dispatchControlCommand("auto_rotate_toggle");
    }
  }

  const String snapshot = snapshotHandler_ ? snapshotHandler_(controlContext_) : String("");
  String json;
  json.reserve(420);
  json += F("{\"brightness\":");
  json += String(snapshotInt(snapshot, "Brightness", 0));
  json += F(",\"dimBrightness\":");
  json += String(snapshotInt(snapshot, "Dim brightness", 0));
  json += F(",\"dimTimeoutSeconds\":");
  json += String(snapshotInt(snapshot, "Dim timeout", 0));
  json += F(",\"sleepTimeoutSeconds\":");
  json += String(snapshotInt(snapshot, "Sleep timeout", 0));
  json += F(",\"lowPowerFace\":");
  json += snapshotOn(snapshot, "Low-power face") ? F("true") : F("false");
  json += F(",\"autoRotate\":\"");
  json += escapeJson(snapshotValue(snapshot, "Auto rotate"));
  json += F("\",\"theme\":\"");
  json += escapeJson(snapshotValue(snapshot, "Theme"));
  json += F("\",\"layout\":\"");
  json += escapeJson(snapshotValue(snapshot, "Face layout"));
  json += F("\"}");
  server_.send(200, "application/json", json);
}

void WifiService::handleApiPowerSettings() {
  if (server_.method() == HTTP_PUT) {
    int intValue = 0;
    bool boolValue = false;
    const String snapshot = snapshotHandler_ ? snapshotHandler_(controlContext_) : String("");
    if (apiIntArg("dimBrightness", &intValue)) {
      dispatchControlCommand(String("dim_brightness_set:") + intValue);
    }
    if (apiIntArg("dimTimeoutSeconds", &intValue)) {
      dispatchControlCommand(String("dim_set:") + intValue);
    }
    if (apiIntArg("sleepTimeoutSeconds", &intValue)) {
      dispatchControlCommand(String("sleep_set:") + intValue);
    }
    if (apiBoolArg("lowPowerFace", &boolValue) &&
        boolValue != snapshotOn(snapshot, "Low-power face")) {
      dispatchControlCommand("low_face_toggle");
    }
    if (apiBoolArg("wifiOnDemand", &boolValue) &&
        boolValue != snapshotOn(snapshot, "WiFi on demand")) {
      dispatchControlCommand("wifi_demand_toggle");
    }
    if (apiBoolArg("indicatorLight", &boolValue) &&
        boolValue != snapshotOn(snapshot, "Indicator light")) {
      dispatchControlCommand("indicator_toggle");
    }
    if (apiBoolArg("nextProfile", &boolValue) && boolValue) {
      dispatchControlCommand("power_profile_next");
    }
  }

  const String snapshot = snapshotHandler_ ? snapshotHandler_(controlContext_) : String("");
  String json;
  json.reserve(360);
  json += F("{\"displayPower\":\"");
  json += escapeJson(snapshotValue(snapshot, "Display power"));
  json += F("\",\"powerProfile\":\"");
  json += escapeJson(snapshotValue(snapshot, "Power profile"));
  json += F("\",\"cpu\":\"");
  json += escapeJson(snapshotValue(snapshot, "CPU"));
  json += F("\",\"dimBrightness\":");
  json += String(snapshotInt(snapshot, "Dim brightness", 0));
  json += F(",\"dimTimeoutSeconds\":");
  json += String(snapshotInt(snapshot, "Dim timeout", 0));
  json += F(",\"sleepTimeoutSeconds\":");
  json += String(snapshotInt(snapshot, "Sleep timeout", 0));
  json += F(",\"wifiOnDemand\":");
  json += snapshotOn(snapshot, "WiFi on demand") ? F("true") : F("false");
  json += F(",\"indicatorLight\":");
  json += snapshotOn(snapshot, "Indicator light") ? F("true") : F("false");
  json += F("}");
  server_.send(200, "application/json", json);
}

void WifiService::handleApiTouchSettings() {
  int value = 0;
  if (server_.method() == HTTP_PUT && apiIntArg("touchDelayMs", &value)) {
    dispatchControlCommand(String("touch_set:") + value);
  }

  const String snapshot = snapshotHandler_ ? snapshotHandler_(controlContext_) : String("");
  String json;
  json.reserve(80);
  json += F("{\"touchDelayMs\":");
  json += String(snapshotInt(snapshot, "Touch delay", 0));
  json += F("}");
  server_.send(200, "application/json", json);
}

void WifiService::handleApiSoundSettings() {
  int value = 0;
  if (server_.method() == HTTP_PUT && apiIntArg("volumePercent", &value)) {
    dispatchControlCommand(String("volume_set:") + value);
  }

  const String snapshot = snapshotHandler_ ? snapshotHandler_(controlContext_) : String("");
  String json;
  json.reserve(90);
  json += F("{\"volumePercent\":");
  json += String(snapshotInt(snapshot, "Volume", 0));
  json += F("}");
  server_.send(200, "application/json", json);
}

void WifiService::handleApiThemeSettings() {
  if (server_.method() == HTTP_PUT) {
    int intValue = 0;
    bool boolValue = false;
    const String snapshot = snapshotHandler_ ? snapshotHandler_(controlContext_) : String("");
    const String widgets = snapshotValue(snapshot, "Widgets");
    const bool batteryWidget = widgets.indexOf("Battery") >= 0;
    const bool dateWidget = widgets.indexOf("Date") >= 0;
    const bool secondsWidget = widgets.indexOf("Seconds") >= 0;
    const bool wifiWidget = widgets.indexOf("WiFi") >= 0;
    const bool complication = snapshotValue(snapshot, "Complication") != "Off";

    if (apiIntArg("themeId", &intValue)) {
      dispatchControlCommand(String("theme_") + intValue);
    }
    if (apiBoolArg("nextTheme", &boolValue) && boolValue) {
      dispatchControlCommand("bg_next");
    }
    if (apiBoolArg("batteryWidget", &boolValue) && boolValue != batteryWidget) {
      dispatchControlCommand("widget_battery_toggle");
    }
    if (apiBoolArg("dateWidget", &boolValue) && boolValue != dateWidget) {
      dispatchControlCommand("widget_date_toggle");
    }
    if (apiBoolArg("secondsWidget", &boolValue) && boolValue != secondsWidget) {
      dispatchControlCommand("widget_seconds_toggle");
    }
    if (apiBoolArg("wifiWidget", &boolValue) && boolValue != wifiWidget) {
      dispatchControlCommand("widget_wifi_toggle");
    }
    if (apiBoolArg("complication", &boolValue) && boolValue != complication) {
      dispatchControlCommand("complication_next");
    }
    if (apiBoolArg("nextComplication", &boolValue) && boolValue) {
      dispatchControlCommand("complication_next");
    }
  }

  const String snapshot = snapshotHandler_ ? snapshotHandler_(controlContext_) : String("");
  const String widgets = snapshotValue(snapshot, "Widgets");
  String json;
  json.reserve(320);
  json += F("{\"theme\":\"");
  json += escapeJson(snapshotValue(snapshot, "Theme"));
  json += F("\",\"layout\":\"");
  json += escapeJson(snapshotValue(snapshot, "Face layout"));
  json += F("\",\"widgets\":\"");
  json += escapeJson(widgets);
  json += F("\",\"batteryWidget\":");
  json += widgets.indexOf("Battery") >= 0 ? F("true") : F("false");
  json += F(",\"dateWidget\":");
  json += widgets.indexOf("Date") >= 0 ? F("true") : F("false");
  json += F(",\"secondsWidget\":");
  json += widgets.indexOf("Seconds") >= 0 ? F("true") : F("false");
  json += F(",\"wifiWidget\":");
  json += widgets.indexOf("WiFi") >= 0 ? F("true") : F("false");
  json += F(",\"complication\":\"");
  json += escapeJson(snapshotValue(snapshot, "Complication"));
  json += F("\"}");
  server_.send(200, "application/json", json);
}

void WifiService::handleApiCommand() {
  String command;
  if (!apiStringArg("command", &command)) {
    apiStringArg("cmd", &command);
  }
  if (command.isEmpty() && server_.uri() == "/api/wifi/setup") {
    command = "wifi_setup";
  }
  if (command.isEmpty()) {
    sendApiError(400, "Missing command.");
    return;
  }
  if (!dispatchControlCommand(command)) {
    sendApiError(503, "Control command handler unavailable.");
    return;
  }
  sendApiOk("Command accepted.");
}

void WifiService::handleApiWifiStatus() {
  if (server_.method() == HTTP_POST) {
    bool boolValue = false;
    const String snapshot = snapshotHandler_ ? snapshotHandler_(controlContext_) : String("");
    if (apiBoolArg("enabled", &boolValue) && boolValue != isEnabled()) {
      dispatchControlCommand("wifi_toggle");
    }
    if (apiBoolArg("onDemand", &boolValue) &&
        boolValue != snapshotOn(snapshot, "WiFi on demand")) {
      dispatchControlCommand("wifi_demand_toggle");
    }
    if (apiBoolArg("setup", &boolValue) && boolValue) {
      dispatchControlCommand("wifi_setup");
    }
  }

  const String snapshot = snapshotHandler_ ? snapshotHandler_(controlContext_) : String("");
  String json;
  json.reserve(260);
  json += F("{\"enabled\":");
  json += isEnabled() ? F("true") : F("false");
  json += F(",\"connected\":");
  json += isConnected() ? F("true") : F("false");
  json += F(",\"provisioning\":");
  json += isProvisioning() ? F("true") : F("false");
  json += F(",\"status\":\"");
  json += escapeJson(snapshotValue(snapshot, "WiFi"));
  json += F("\",\"ssid\":\"");
  json += escapeJson(snapshotValue(snapshot, "SSID"));
  json += F("\",\"ip\":\"");
  json += escapeJson(snapshotValue(snapshot, "IP"));
  json += F("\"}");
  server_.send(200, "application/json", json);
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

void WifiService::appendPageShellStart(String& html, const String& page, const String& snapshot) {
  html += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<meta http-equiv='refresh' content='20'><title>Iris</title><style>");
  html += F(":root{color-scheme:dark}body{font-family:system-ui,-apple-system,Segoe UI,sans-serif;background:#080908;color:#f5f7f2;margin:0}a{color:inherit}.wrap{max-width:1080px;margin:0 auto;padding:20px}.top{display:grid;grid-template-columns:220px 1fr;gap:22px;align-items:center}.watch{width:190px;height:190px;border-radius:50%;background:#000;border:8px solid #202420;display:grid;place-items:center;box-shadow:0 0 0 1px #3b433b,0 16px 36px #0008}.face{text-align:center}.time{font-size:42px;font-weight:800;line-height:1}.date{margin-top:12px;color:#b7c5b8}.chip{display:inline-block;margin-top:14px;border:1px solid #344035;border-radius:999px;padding:5px 10px;color:#c9d7c9;font-size:13px}.title h1{margin:0;font-size:36px}.title p{color:#aab5aa;max-width:680px}.layout{display:grid;grid-template-columns:210px minmax(0,1fr);gap:22px;margin-top:22px}nav{display:flex;flex-direction:column;gap:8px}.nav{padding:12px 14px;border:1px solid #283028;border-radius:8px;text-decoration:none;background:#111611;color:#d8e2d8}.nav.active{background:#d9f99d;color:#111;border-color:#d9f99d;font-weight:800}section{background:#101410;border:1px solid #283028;border-radius:8px;padding:18px}h2{margin:0 0 16px}h3{margin:20px 0 10px}.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}.grid.three{grid-template-columns:repeat(3,minmax(0,1fr))}.button,button{display:block;box-sizing:border-box;width:100%;padding:12px 13px;border-radius:8px;border:1px solid #3a453a;background:#1a211a;color:#fff;text-align:center;text-decoration:none;font-size:15px}.button.warn{border-color:#f0c36a;background:#342710}.control{border:1px solid #283028;border-radius:8px;padding:14px;margin:12px 0;background:#0b0e0b}.control label{display:flex;justify-content:space-between;gap:10px;font-weight:700}.control input[type=range]{width:100%;margin:14px 0}.control input[type=number]{width:82px;background:#050605;color:#fff;border:1px solid #3a453a;border-radius:8px;padding:9px}.control form{display:grid;grid-template-columns:1fr auto auto;gap:10px;align-items:center}.facts{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px;margin-bottom:14px}.facts p{margin:0;padding:12px;border:1px solid #283028;border-radius:8px;background:#0b0e0b}.facts b{display:block;color:#9faf9f;font-size:12px;text-transform:uppercase}.facts span{display:block;margin-top:6px;font-size:18px}pre{white-space:pre-wrap;background:#050605;border:1px solid #283028;border-radius:8px;padding:12px;color:#cfd8cf}.hint{color:#aab5aa;font-size:14px}.on{border-color:#9ee493;background:#18321d}@media(max-width:760px){.top,.layout{grid-template-columns:1fr}.watch{margin:auto}nav{display:grid;grid-template-columns:repeat(2,minmax(0,1fr))}.grid,.grid.three,.facts{grid-template-columns:1fr}.control form{grid-template-columns:1fr}}</style></head><body><div class='wrap'>");
  html += F("<div class='top'>");
  appendWatchPreview(html, snapshot);
  html += F("<div class='title'><h1>Iris</h1><p>Web configurator for display, touch, sound, WiFi, theme, power, device, and development controls.</p></div></div><div class='layout'>");
  appendNavigation(html, page);
  html += F("<main>");
}

void WifiService::appendPageShellEnd(String& html) {
  html += F("</main></div></div></body></html>");
}

void WifiService::appendWatchPreview(String& html, const String& snapshot) {
  String timeText = snapshotValue(snapshot, "Time");
  if (timeText.isEmpty()) timeText = "--:--";
  html += F("<div class='watch'><div class='face'><div class='time'>");
  html += escapeHtml(timeText);
  html += F("</div><div class='date'>");
  html += escapeHtml(snapshotValue(snapshot, "Theme"));
  html += F("</div><div class='chip'>");
  html += escapeHtml(snapshotValue(snapshot, "Battery"));
  html += F("</div></div></div>");
}

void WifiService::appendNavigation(String& html, const String& page) {
  constexpr const char* pages[] = {"dashboard", "display", "touch", "sound", "wifi", "theme", "power", "device", "development"};
  constexpr const char* labels[] = {"Dashboard", "Display", "Touch", "Sound", "WiFi", "Theme", "Power", "Device", "Development"};
  html += F("<nav>");
  for (size_t i = 0; i < sizeof(pages) / sizeof(pages[0]); ++i) {
    html += F("<a class='nav");
    if (page == pages[i] || (page.isEmpty() && i == 0)) html += F(" active");
    html += F("' href='/?page=");
    html += pages[i];
    html += F("'>");
    html += labels[i];
    html += F("</a>");
  }
  html += F("</nav>");
}

void WifiService::appendRangeControl(String& html, const char* label, const char* command,
                                     int value, int minValue, int maxValue, int step,
                                     const char* suffix) {
  const String page = server_.arg("page");
  html += F("<div class='control'><form method='get' action='/control'><input type='hidden' name='cmd' value='");
  html += command;
  html += F("'><input type='hidden' name='page' value='");
  html += escapeHtml(page);
  html += F("'><label>");
  html += label;
  html += F("<span>");
  html += String(value);
  html += suffix;
  html += F("</span></label><input type='range' value='");
  html += String(value);
  html += F("' min='");
  html += String(minValue);
  html += F("' max='");
  html += String(maxValue);
  html += F("' step='");
  html += String(step);
  html += F("' oninput='this.form.value.value=this.value'><input name='value' type='number' value='");
  html += String(value);
  html += F("' min='");
  html += String(minValue);
  html += F("' max='");
  html += String(maxValue);
  html += F("' step='");
  html += String(step);
  html += F("'><button type='submit'>Apply</button></form></div>");
}

void WifiService::appendToggleControl(String& html, const char* label, const char* command,
                                      bool enabled) {
  html += F("<div class='control'><label>");
  html += label;
  html += F("<span>");
  html += enabled ? F("On") : F("Off");
  html += F("</span></label>");
  appendAction(html, enabled ? "Turn off" : "Turn on", command, enabled ? "on" : "");
  html += F("</div>");
}

void WifiService::appendAction(String& html, const char* label, const char* command,
                               const char* className) {
  const String page = server_.arg("page");
  html += F("<a class='button ");
  html += className;
  html += F("' href='/control?cmd=");
  html += command;
  html += F("&page=");
  html += escapeHtml(page);
  html += F("'>");
  html += label;
  html += F("</a>");
}

String WifiService::snapshotValue(const String& snapshot, const char* key) const {
  String prefix = String(key) + ": ";
  int start = snapshot.indexOf(prefix);
  if (start < 0) return "";
  start += prefix.length();
  int end = snapshot.indexOf('\n', start);
  if (end < 0) end = snapshot.length();
  String value = snapshot.substring(start, end);
  value.trim();
  return value;
}

int WifiService::snapshotInt(const String& snapshot, const char* key, int fallback) const {
  const String value = snapshotValue(snapshot, key);
  if (value.isEmpty() || value == "Off") return 0;
  int result = 0;
  bool found = false;
  for (size_t i = 0; i < value.length(); ++i) {
    if (isDigit(value[i])) {
      result = (result * 10) + (value[i] - '0');
      found = true;
    } else if (found) {
      break;
    }
  }
  return found ? result : fallback;
}

bool WifiService::snapshotOn(const String& snapshot, const char* key) const {
  const String value = snapshotValue(snapshot, key);
  return value == "On" || value.startsWith("On ");
}

bool WifiService::dispatchControlCommand(const String& command) {
  if (!commandHandler_ || command.isEmpty()) return false;
  commandHandler_(controlContext_, command);
  return true;
}

String WifiService::apiArg(const char* name) {
  if (server_.hasArg(name)) {
    return server_.arg(name);
  }

  const String body = server_.arg("plain");
  if (body.isEmpty()) return "";

  String needle = String("\"") + name + "\"";
  int key = body.indexOf(needle);
  if (key < 0) key = body.indexOf(name);
  if (key < 0) return "";

  int start = body.indexOf(':', key);
  if (start < 0) return "";
  ++start;
  while (start < static_cast<int>(body.length()) && isspace(body[start])) {
    ++start;
  }
  if (start >= static_cast<int>(body.length())) return "";

  if (body[start] == '"') {
    ++start;
    int end = body.indexOf('"', start);
    if (end < 0) end = body.length();
    return body.substring(start, end);
  }

  int end = start;
  while (end < static_cast<int>(body.length()) &&
         body[end] != ',' && body[end] != '}' && body[end] != '\r' && body[end] != '\n') {
    ++end;
  }
  String value = body.substring(start, end);
  value.trim();
  return value;
}

bool WifiService::apiIntArg(const char* name, int* value) {
  if (!value) return false;
  const String raw = apiArg(name);
  if (raw.isEmpty()) return false;
  *value = raw.toInt();
  return true;
}

bool WifiService::apiBoolArg(const char* name, bool* value) {
  if (!value) return false;
  String raw = apiArg(name);
  raw.toLowerCase();
  if (raw == "true" || raw == "1" || raw == "on" || raw == "yes") {
    *value = true;
    return true;
  }
  if (raw == "false" || raw == "0" || raw == "off" || raw == "no") {
    *value = false;
    return true;
  }
  return false;
}

bool WifiService::apiStringArg(const char* name, String* value) {
  if (!value) return false;
  *value = apiArg(name);
  value->trim();
  return !value->isEmpty();
}

void WifiService::sendApiError(int code, const char* message) {
  String json;
  json.reserve(80);
  json += F("{\"ok\":false,\"error\":\"");
  json += escapeJson(message);
  json += F("\"}");
  server_.send(code, "application/json", json);
}

void WifiService::sendApiOk(const char* message) {
  String json;
  json.reserve(80);
  json += F("{\"ok\":true,\"message\":\"");
  json += escapeJson(message);
  json += F("\"}");
  server_.send(200, "application/json", json);
}

String WifiService::escapeJson(const String& value) {
  String escaped;
  escaped.reserve(value.length() + 8);
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value[i];
    if (c == '\\' || c == '"') {
      escaped += '\\';
      escaped += c;
    } else if (c == '\n') {
      escaped += F("\\n");
    } else if (c == '\r') {
      escaped += F("\\r");
    } else {
      escaped += c;
    }
  }
  return escaped;
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
