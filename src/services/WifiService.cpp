#include "iris/services/WifiService.h"

#include "iris/AppConfig.h"

namespace iris {

namespace {
const char* apiEndpointForCommand(const char* command) {
  if (strcmp(command, "brightness_set") == 0 ||
      strcmp(command, "dim_brightness_set") == 0 ||
      strcmp(command, "dim_set") == 0 ||
      strcmp(command, "sleep_set") == 0) {
    return "/api/settings/display";
  }
  if (strcmp(command, "touch_set") == 0) return "/api/settings/touch";
  if (strcmp(command, "volume_set") == 0) return "/api/settings/sound";
  return "";
}

const char* apiFieldForCommand(const char* command) {
  if (strcmp(command, "brightness_set") == 0) return "brightness";
  if (strcmp(command, "dim_brightness_set") == 0) return "dimBrightness";
  if (strcmp(command, "dim_set") == 0) return "dimTimeoutSeconds";
  if (strcmp(command, "sleep_set") == 0) return "sleepTimeoutSeconds";
  if (strcmp(command, "touch_set") == 0) return "touchDelayMs";
  if (strcmp(command, "volume_set") == 0) return "volumePercent";
  return "";
}

const char* previewStyleForTheme(const String& theme) {
  if (theme == "Midnight") return "background:#000012;color:#ffffff;border-color:#4b4fff;--muted:#b5b8ff;--accent:#9ea3ff;--panel:#101443";
  if (theme == "Forest") return "background:#001b09;color:#ffffff;border-color:#76d676;--muted:#c7f5c7;--accent:#94e094;--panel:#0d3417";
  if (theme == "Plum") return "background:#240024;color:#ffffff;border-color:#e18ce1;--muted:#ffd0ff;--accent:#f4a8f4;--panel:#3a123a";
  if (theme == "Steel") return "background:#1f2428;color:#ffffff;border-color:#bfcbd3;--muted:#dbe6ec;--accent:#d2dde4;--panel:#30383f";
  return "background:#000000;color:#ffffff;border-color:#3b433b;--muted:#c9d7c9;--accent:#d9f99d;--panel:#171b17";
}

struct WebThemeOption {
  const char* name;
  const char* layout;
  const char* background;
  const char* foreground;
  const char* muted;
  const char* accent;
  const char* panel;
};

constexpr WebThemeOption kWebThemeOptions[] = {
    {"Black", "Classic", "#000000", "#ffffff", "#c9d7c9", "#d9f99d", "#171b17"},
    {"Midnight", "Focus", "#000012", "#ffffff", "#b5b8ff", "#9ea3ff", "#101443"},
    {"Forest", "Compact", "#001b09", "#ffffff", "#c7f5c7", "#94e094", "#0d3417"},
    {"Plum", "Classic", "#240024", "#ffffff", "#ffd0ff", "#f4a8f4", "#3a123a"},
    {"Steel", "Compact", "#1f2428", "#ffffff", "#dbe6ec", "#d2dde4", "#30383f"},
};

int themeIdForName(const String& theme) {
  for (size_t i = 0; i < sizeof(kWebThemeOptions) / sizeof(kWebThemeOptions[0]); ++i) {
    if (theme == kWebThemeOptions[i].name) return static_cast<int>(i);
  }
  return 0;
}

void appendThemeCard(String& html, size_t index, const String& activeTheme) {
  const WebThemeOption& theme = kWebThemeOptions[index];
  const bool active = activeTheme == theme.name;
  html += F("<a class='theme-card");
  if (active) html += F(" active");
  html += F("' href='/control?cmd=theme_");
  html += String(index);
  html += F("&page=theme' data-command='theme_");
  html += String(index);
  html += F("'><div class='theme-swatch' style='background:");
  html += theme.background;
  html += F(";color:");
  html += theme.foreground;
  html += F(";border-color:");
  html += theme.accent;
  html += F("'><span style='background:");
  html += theme.accent;
  html += F("'></span><span style='background:");
  html += theme.muted;
  html += F("'></span><span style='background:");
  html += theme.panel;
  html += F("'></span></div><b>");
  html += theme.name;
  html += F("</b><small>");
  html += theme.layout;
  html += active ? F(" / active") : F(" layout");
  html += F("</small></a>");
}

const char* encryptionName(wifi_auth_mode_t type) {
  switch (type) {
    case WIFI_AUTH_OPEN: return "open";
    case WIFI_AUTH_WEP: return "wep";
    case WIFI_AUTH_WPA_PSK: return "wpa";
    case WIFI_AUTH_WPA2_PSK: return "wpa2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "wpa/wpa2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "wpa2-enterprise";
    case WIFI_AUTH_WPA3_PSK: return "wpa3";
    case WIFI_AUTH_WPA2_WPA3_PSK: return "wpa2/wpa3";
    default: return "unknown";
  }
}

String formatBytes(uint32_t bytes) {
  if (bytes >= 1024UL * 1024UL) {
    return String(bytes / (1024UL * 1024UL)) + " MB";
  }
  if (bytes >= 1024UL) {
    return String(bytes / 1024UL) + " KB";
  }
  return String(bytes) + " B";
}

String formatUptime(uint32_t nowMs) {
  const uint32_t totalSeconds = nowMs / 1000UL;
  const uint32_t days = totalSeconds / 86400UL;
  const uint32_t hours = (totalSeconds / 3600UL) % 24UL;
  const uint32_t minutes = (totalSeconds / 60UL) % 60UL;
  const uint32_t seconds = totalSeconds % 60UL;
  char buffer[24];
  if (days > 0) {
    snprintf(buffer, sizeof(buffer), "%lud %02lu:%02lu:%02lu",
             static_cast<unsigned long>(days),
             static_cast<unsigned long>(hours),
             static_cast<unsigned long>(minutes),
             static_cast<unsigned long>(seconds));
  } else {
    snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu",
             static_cast<unsigned long>(hours),
             static_cast<unsigned long>(minutes),
             static_cast<unsigned long>(seconds));
  }
  return String(buffer);
}
}  // namespace

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
    serverRunning_ = false;
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

void WifiService::disconnectStation() {
  if (!enabled_) return;

  WiFi.disconnect(false, false);
  lastConnectAttemptMs_ = millis();

  if (!portalRunning_) {
    WiFi.mode(WIFI_STA);
    ensureServer();
  }
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

void WifiService::forgetSavedNetwork() {
  savedSsid_ = "";
  savedPassword_ = "";
  prefs_.remove("ssid");
  prefs_.remove("password");

  if (portalRunning_) {
    return;
  }

  WiFi.disconnect(true, false);
  if (enabled_) {
    WiFi.mode(WIFI_STA);
    ensureServer();
  } else {
    WiFi.setSleep(false);
    WiFi.mode(WIFI_OFF);
  }
}

void WifiService::shutdownForBootloader() {
  Serial.println("[BOOT] Stopping web server and WiFi before bootloader transition");
  if (serverRunning_) {
    server_.stop();
    serverRunning_ = false;
  }
  if (portalRunning_) {
    WiFi.softAPdisconnect(true);
    portalRunning_ = false;
  }
  WiFi.disconnect(true, false);
  WiFi.setSleep(false);
  WiFi.mode(WIFI_OFF);
}

void WifiService::configurePortalRoutes() {
  if (routesConfigured_) return;

  server_.on("/", HTTP_GET, [this]() { handleControlPanel(); });
  server_.on("/dashboard", HTTP_GET, [this]() {
    server_.sendHeader("Location", "/?page=dashboard", true);
    server_.send(302, "text/plain", "");
  });
  server_.on("/device", HTTP_GET, [this]() {
    server_.sendHeader("Location", "/?page=device", true);
    server_.send(302, "text/plain", "");
  });
  server_.on("/apps", HTTP_GET, [this]() {
    server_.sendHeader("Location", "/?page=apps", true);
    server_.send(302, "text/plain", "");
  });
  server_.on("/settings", HTTP_GET, [this]() {
    server_.sendHeader("Location", "/?page=settings", true);
    server_.send(302, "text/plain", "");
  });
  server_.on("/display", HTTP_GET, [this]() {
    server_.sendHeader("Location", "/?page=display", true);
    server_.send(302, "text/plain", "");
  });
  server_.on("/theme", HTTP_GET, [this]() {
    server_.sendHeader("Location", "/?page=theme", true);
    server_.send(302, "text/plain", "");
  });
  server_.on("/datetime", HTTP_GET, [this]() {
    server_.sendHeader("Location", "/?page=datetime", true);
    server_.send(302, "text/plain", "");
  });
  server_.on("/touch", HTTP_GET, [this]() {
    server_.sendHeader("Location", "/?page=touch", true);
    server_.send(302, "text/plain", "");
  });
  server_.on("/sound", HTTP_GET, [this]() {
    server_.sendHeader("Location", "/?page=sound", true);
    server_.send(302, "text/plain", "");
  });
  server_.on("/wifi", HTTP_GET, [this]() {
    server_.sendHeader("Location", "/?page=wifi", true);
    server_.send(302, "text/plain", "");
  });
  server_.on("/power", HTTP_GET, [this]() {
    server_.sendHeader("Location", "/?page=power", true);
    server_.send(302, "text/plain", "");
  });
  server_.on("/development", HTTP_GET, [this]() {
    server_.sendHeader("Location", "/?page=development", true);
    server_.send(302, "text/plain", "");
  });
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
  server_.on("/api/settings/time", HTTP_GET, [this]() { handleApiTimeSettings(); });
  server_.on("/api/settings/time", HTTP_PUT, [this]() { handleApiTimeSettings(); });
  server_.on("/api/device", HTTP_GET, [this]() { handleApiDeviceInfo(); });
  server_.on("/api/apps", HTTP_GET, [this]() { handleApiApps(); });
  server_.on("/api/command", HTTP_POST, [this]() { handleApiCommand(); });
  server_.on("/api/wifi/status", HTTP_GET, [this]() { handleApiWifiStatus(); });
  server_.on("/api/wifi/status", HTTP_POST, [this]() { handleApiWifiStatus(); });
  server_.on("/api/wifi/networks", HTTP_GET, [this]() { handleApiWifiNetworks(); });
  server_.on("/api/wifi/scan", HTTP_POST, [this]() { handleApiWifiNetworks(); });
  server_.on("/api/wifi/connect", HTTP_POST, [this]() { handleApiWifiConnect(); });
  server_.on("/api/wifi/disconnect", HTTP_POST, [this]() { handleApiWifiDisconnect(); });
  server_.on("/api/wifi/reconnect", HTTP_POST, [this]() { handleApiWifiReconnect(); });
  server_.on("/api/wifi/forget", HTTP_POST, [this]() { handleApiWifiForget(); });
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
  html.reserve(18000);
  appendPageShellStart(html, page, snapshot);

  if (page == "settings") {
    html += F("<section><h2>Settings</h2><p class='hint'>Choose a settings area. These pages use the same Iris settings model as the watch UI, but leave more room for precise controls.</p><div class='grid'>");
    html += F("<a class='button' href='/?page=display'>Display</a>");
    html += F("<a class='button' href='/?page=theme'>Theme & Widgets</a>");
    html += F("<a class='button' href='/?page=datetime'>Date & Time</a>");
    html += F("<a class='button' href='/?page=touch'>Touch</a>");
    html += F("<a class='button' href='/?page=sound'>Sound</a>");
    html += F("<a class='button' href='/?page=wifi'>WiFi</a>");
    html += F("<a class='button' href='/?page=power'>Power</a>");
    html += F("<a class='button' href='/?page=device'>Device</a>");
    html += F("</div></section>");
  } else if (page == "display") {
    html += F("<section><h2>Display</h2><p class='hint'>Fine tune the AMOLED display and watch-face power behavior.</p>");
    html += F("<h3>Display</h3>");
    appendRangeControl(html, "Brightness", "brightness_set", snapshotInt(snapshot, "Brightness", 96), 16, 255, 1, "/255");
    appendRangeControl(html, "Dim brightness", "dim_brightness_set", snapshotInt(snapshot, "Dim brightness", 18), 1, 96, 1, "/255");
    appendRangeControl(html, "Dim timeout", "dim_set", snapshotInt(snapshot, "Dim timeout", 20), 5, 120, 1, " sec");
    appendRangeControl(html, "Sleep timeout", "sleep_set", snapshotInt(snapshot, "Sleep timeout", 90), 0, 600, 5, " sec");
    appendToggleControl(html, "Low-power watch face", "low_face_toggle", snapshotOn(snapshot, "Low-power face"));
    appendToggleControl(html, "Automatic rotation", "auto_rotate_toggle", snapshotValue(snapshot, "Auto rotate") != "Off");
    html += F("</section>");
  } else if (page == "theme") {
    const String activeTheme = snapshotValue(snapshot, "Theme");
    html += F("<section><h2>Theme & Widgets</h2><p class='hint'>Change the active watch theme, review its colors, and choose visible watch-face widgets.</p>");
    html += F("<h3>Active Appearance</h3><div class='facts'>");
    html += F("<p><b>Theme</b><span>");
    html += escapeHtml(activeTheme);
    html += F("</span></p><p><b>Layout</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Face layout"));
    html += F("</span></p><p><b>Widgets</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Widgets"));
    html += F("</span></p><p><b>Complication</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Complication"));
    html += F("</span></p></div><h3>Built-in Themes</h3><div class='theme-list'>");
    for (size_t i = 0; i < sizeof(kWebThemeOptions) / sizeof(kWebThemeOptions[0]); ++i) {
      appendThemeCard(html, i, activeTheme);
    }
    html += F("</div><div class='grid'>");
    appendAction(html, "Next theme", "bg_next");
    html += F("</div><h3>Widgets</h3><div class='grid'>");
    appendAction(html, "Toggle battery", "widget_battery_toggle");
    appendAction(html, "Toggle date", "widget_date_toggle");
    appendAction(html, "Toggle seconds", "widget_seconds_toggle");
    appendAction(html, "Toggle WiFi", "widget_wifi_toggle");
    appendAction(html, "Next complication", "complication_next");
    html += F("</div>");
    html += F("</section>");
  } else if (page == "datetime") {
    html += F("<section><h2>Date & Time</h2><p class='hint'>Configure regional formats, timezone, NTP sync, manual time, and RTC behavior.</p>");
    html += F("<h3>Date & Time</h3><div class='facts'>");
    html += F("<p><b>Country</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Country"));
    html += F("</span></p><p><b>Locale</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Locale"));
    html += F("</span></p><p><b>Date format</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Date format"));
    html += F("</span></p><p><b>Time format</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Time format"));
    html += F("</span></p><p><b>Time zone</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Time zone"));
    html += F("</span></p><p><b>Sync</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Time sync"));
    html += F("</span></p></div><div class='grid'>");
    appendAction(html, "Next country", "country_next");
    appendAction(html, "Next time zone", "timezone_next");
    appendAction(html, "Next date format", "date_format_next");
    appendAction(html, "Next time format", "time_format_next");
    appendAction(html, "Sync now", "time_sync_now");
    appendAction(html, "Open Date & Time", "date_time");
    html += F("</div>");
    appendToggleControl(html, "Automatic date & time", "auto_time_toggle",
                        snapshotOn(snapshot, "Automatic time"));
    html += F("<div class='control'><form method='get' action='/control'><input type='hidden' name='cmd' value='manual_time_set'><input type='hidden' name='page' value='datetime'><label>Manual date & time<span>");
    html += snapshotOn(snapshot, "Automatic time") ? F("Turn auto off first") : F("System + RTC");
    html += F("</span></label><input name='value' type='datetime-local'><button type='submit'>Apply</button></form></div>");
    html += F("<p class='hint'>Country applies date and time format defaults only. Time zone remains a separate setting. Manual date/time applies only when automatic date & time is off.</p>");
    html += F("</section>");
  } else if (page == "touch") {
    html += F("<section><h2>Touch</h2><p class='hint'>Tune touch delay and selection feel for the small round display.</p>");
    html += F("<h3>Touch</h3>");
    appendRangeControl(html, "Touch delay", "touch_set", snapshotInt(snapshot, "Touch delay", 150), 50, 500, 5, " ms");
    html += F("<p class='hint'>Menu screens still enforce a slightly longer minimum hold so scrolling does not accidentally select an item.</p>");
    html += F("</section>");
  } else if (page == "sound") {
    html += F("<section><h2>Sound</h2><p class='hint'>Adjust master audio output and quick volume controls.</p>");
    html += F("<h3>Sound</h3>");
    appendRangeControl(html, "Master volume", "volume_set", snapshotInt(snapshot, "Volume", 38), 0, 100, 1, "%");
    appendAction(html, "Volume -", "vol_down");
    appendAction(html, "Volume +", "vol_up");
    html += F("</section>");
  } else if (page == "wifi") {
    html += F("<section><h2>WiFi</h2><p class='hint'>Manage connection state, on-demand radio behavior, and setup portal access.</p>");
    html += F("<h3>WiFi</h3><div class='facts'>");
    html += F("<p><b>Status</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "WiFi"));
    html += F("</span></p><p><b>Network</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "SSID"));
    html += F("</span></p><p><b>IP Address</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "IP"));
    html += F("</span></p></div>");
    appendToggleControl(html, "WiFi enabled", "wifi_toggle", isEnabled());
    appendToggleControl(html, "WiFi on demand", "wifi_demand_toggle", snapshotOn(snapshot, "WiFi on demand"));
    html += F("<h3>Connection Actions</h3><div class='grid'>");
    html += F("<button type='button' data-wifi-action='/api/wifi/connect'>Connect saved</button>");
    html += F("<button type='button' data-wifi-action='/api/wifi/reconnect'>Reconnect</button>");
    html += F("<button type='button' class='warn' data-wifi-action='/api/wifi/disconnect' data-confirm='Disconnect from the current WiFi network? The web page may disconnect until Iris reconnects.'>Disconnect</button>");
    html += F("</div>");
    appendAction(html, "Start setup AP", "wifi_setup", "warn");
    html += F("<a class='button' href='/setup'>Choose network</a>");
    html += F("<h3>Saved Network</h3><div class='control'><label>Saved credentials<span>");
    if (hasSavedNetwork()) {
      html += escapeHtml(savedSsid_);
    } else {
      html += F("None");
    }
    html += F("</span></label><button type='button' class='warn' id='wifi-forget-button'");
    if (!hasSavedNetwork()) html += F(" disabled");
    html += F(">Forget saved network</button><p class='hint'>This removes the saved SSID and password from Iris. If you are using this WiFi connection now, the web page may disconnect after it succeeds.</p></div>");
    html += F("<h3>Nearby Networks</h3><div class='control'><label>Network scan<span id='wifi-scan-status'>Ready</span></label><button type='button' id='wifi-scan-button'>Scan networks</button><div id='wifi-networks' class='network-list'></div><p class='hint'>Passwords are never displayed. Use Choose network to save credentials.</p><a class='button' href='/api/wifi/networks'>Nearby networks JSON</a></div>");
    html += F("</section>");
  } else if (page == "power") {
    html += F("<section><h2>Power</h2><p class='hint'>Balance responsiveness, heat, and battery life.</p>");
    html += F("<h3>Power</h3>");
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
    html += F("<section><h2>Device</h2><p class='hint'>Hardware, runtime, connectivity, and firmware details for this Iris build.</p>");
    html += F("<h3>Identity</h3><div class='facts'>");
    html += F("<p><b>Project</b><span>Iris</span></p><p><b>Firmware</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Firmware"));
    html += F("</span></p><p><b>Hardware</b><span>M5Stack StopWatch</span></p><p><b>MCU</b><span>");
    html += escapeHtml(String(ESP.getChipModel()));
    html += F("</span></p><p><b>MAC address</b><span>");
    html += escapeHtml(WiFi.macAddress());
    html += F("</span></p><p><b>IP address</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "IP"));
    html += F("</span></p></div><h3>Runtime</h3><div class='facts'>");
    html += F("<p><b>Current screen</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Screen"));
    html += F("</span></p><p><b>Current app</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "App"));
    html += F("</span></p><p><b>App state</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "App state"));
    html += F("</span></p><p><b>Uptime</b><span>");
    html += formatUptime(millis());
    html += F("</span></p><p><b>CPU</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "CPU"));
    html += F("</span></p><p><b>Free heap</b><span>");
    html += formatBytes(ESP.getFreeHeap());
    html += F("</span></p></div><h3>Power & Display</h3><div class='facts'>");
    html += F("<p><b>Battery</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Battery"));
    html += F("</span></p><p><b>Display power</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Display power"));
    html += F("</span></p><p><b>Power profile</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Power profile"));
    html += F("</span></p><p><b>Loop delay</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Loop delay"));
    html += F("</span></p><p><b>Foreground update</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Foreground update"));
    html += F("</span></p><p><b>Theme</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Theme"));
    html += F("</span></p><p><b>Face layout</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Face layout"));
    html += F("</span></p></div><h3>Memory & Services</h3><div class='facts'>");
    html += F("<p><b>Flash</b><span>");
    html += formatBytes(ESP.getFlashChipSize());
    html += F("</span></p><p><b>PSRAM</b><span>");
    html += formatBytes(ESP.getPsramSize());
    html += F("</span></p><p><b>Registered apps</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Registered apps"));
    html += F("</span></p><p><b>Services</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Services"));
    html += F("</span></p><p><b>Events</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Events"));
    html += F("</span></p><p><b>WiFi service</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "WiFi service"));
    html += F("</span></p></div><h3>Connectivity</h3><div class='facts'>");
    html += F("<p><b>WiFi</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "WiFi"));
    html += F("</span></p><p><b>SSID</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "SSID"));
    html += F("</span></p><p><b>IP</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "IP"));
    html += F("</span></p><p><b>MAC</b><span>");
    html += escapeHtml(WiFi.macAddress());
    html += F("</span></p></div><h3>Raw snapshot</h3><pre>");
    html += escapeHtml(snapshot);
    html += F("</pre><a class='button' href='/display.txt'>Plain text snapshot</a><a class='button' href='/api/device'>Device API</a><a class='button' href='/api/apps'>Apps API</a><a class='button' href='/api/settings'>JSON settings API</a><a class='button' href='/api/settings/display'>Display API</a><a class='button' href='/api/settings/time'>Date & Time API</a><a class='button' href='/api/settings/touch'>Touch API</a><a class='button' href='/api/settings/sound'>Sound API</a><a class='button' href='/api/settings/theme'>Theme API</a><a class='button' href='/api/settings/power'>Power API</a><a class='button' href='/api/wifi/status'>WiFi API</a><a class='button' href='/api/wifi/networks'>WiFi networks API</a></section>");
  } else if (page == "apps") {
    html += F("<section><h2>Apps</h2><p class='hint'>Registered Iris apps and app-facing screens. Future app settings can use this same web surface.</p>");
    html += F("<h3>Current App</h3><div class='facts'>");
    html += F("<p><b>ID</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "App"));
    html += F("</span></p><p><b>Kind</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "App kind"));
    html += F("</span></p><p><b>State</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "App state"));
    html += F("</span></p><p><b>Registered</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Registered apps"));
    html += F("</span></p></div><h3>Registry</h3>");
    appendAppRegistry(html, snapshotValue(snapshot, "App registry"));
    html += F("<a class='button' href='/api/apps'>Apps API</a></section>");
  } else if (page == "development") {
    html += F("<section><h2>Development</h2><h3>Tools</h3><div class='grid'>");
    appendAction(html, "Hardware Diagnostics", "hardware_diagnostics");
    appendAction(html, "Boot into Bootloader", "bootloader_confirmed", "warn");
    appendAction(html, "Development screen", "development");
    appendAction(html, "Settings screen", "settings");
    html += F("</div><p class='hint'>Bootloader restarts Iris into ESP32-S3 USB download mode. WiFi and this web page will disconnect.</p>");
    html += F("<h3>Remote Input</h3><div class='grid'>");
    appendAction(html, "Watch screen", "watch");
    appendAction(html, "BtnA", "btn_a");
    appendAction(html, "BtnB", "btn_b");
    html += F("</div><h3>Hardware Diagnostics</h3><div class='facts'>");
    html += F("<p><b>System</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "CPU"));
    html += F("</span></p><p><b>Power</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Battery"));
    html += F("</span></p><p><b>WiFi</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "WiFi"));
    html += F("</span></p><p><b>RTC</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Time"));
    html += F("</span></p></div><p class='hint'>Open Hardware Diagnostics on the device for raw IMU, touch, audio, display, RTC, haptic, and power tests.</p></section>");
  } else {
    html += F("<section><h2>Dashboard</h2><h3>Device Status</h3><div class='facts'>");
    html += F("<p><b>Battery</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Battery"));
    html += F("</span></p><p><b>WiFi</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "WiFi"));
    html += F("</span></p><p><b>Network</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "SSID"));
    html += F("</span></p><p><b>IP Address</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "IP"));
    html += F("</span></p><p><b>Date</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Date"));
    html += F("</span></p><p><b>Time</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Time"));
    html += F("</span></p><p><b>RTC / NTP</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Last NTP sync"));
    html += F("</span></p><p><b>Screen</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Screen"));
    html += F("</span></p></div><h3>System Resources</h3><div class='facts'>");
    html += F("<p><b>CPU</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "CPU"));
    html += F("</span></p><p><b>Loop delay</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Loop delay"));
    html += F("</span></p><p><b>Foreground update</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Foreground update"));
    html += F("</span></p><p><b>Services</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Services"));
    html += F("</span></p></div><h3>About Iris</h3><div class='facts'>");
    html += F("<p><b>Project</b><span>Iris</span></p><p><b>Firmware</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Firmware"));
    html += F("</span></p><p><b>Hardware</b><span>M5Stack StopWatch</span></p><p><b>Display</b><span>1.75&quot; AMOLED</span></p><p><b>Resolution</b><span>466 x 466</span></p><p><b>MCU</b><span>ESP32-S3</span></p><p><b>IMU</b><span>BMI270 6-axis</span></p><p><b>Theme</b><span>");
    html += escapeHtml(snapshotValue(snapshot, "Theme"));
    html += F("</span></p></div></section>");
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
  if (command == "bootloader_confirmed") {
    server_.send(200, "text/html",
                 "<!doctype html><html><body style='font-family:system-ui;background:#111;color:#eee;padding:28px'>"
                 "<h1>Entering Bootloader</h1>"
                 "<p>Iris is restarting into firmware download mode. WiFi and this page will disconnect.</p>"
                 "</body></html>");
    delay(200);
    dispatchControlCommand(command);
    return;
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
  json += F("\",\"time\":\"");
  json += escapeJson(snapshotValue(snapshot, "Time"));
  json += F("\",\"date\":\"");
  json += escapeJson(snapshotValue(snapshot, "Date"));
  json += F("\",\"country\":\"");
  json += escapeJson(snapshotValue(snapshot, "Country"));
  json += F("\",\"locale\":\"");
  json += escapeJson(snapshotValue(snapshot, "Locale"));
  json += F("\",\"dateFormat\":\"");
  json += escapeJson(snapshotValue(snapshot, "Date format"));
  json += F("\",\"timeFormat\":\"");
  json += escapeJson(snapshotValue(snapshot, "Time format"));
  json += F("\",\"timeZone\":\"");
  json += escapeJson(snapshotValue(snapshot, "Time zone"));
  json += F("\",\"automaticTime\":\"");
  json += escapeJson(snapshotValue(snapshot, "Automatic time"));
  json += F("\",\"timeSync\":\"");
  json += escapeJson(snapshotValue(snapshot, "Time sync"));
  json += F("\",\"lastNtpSync\":\"");
  json += escapeJson(snapshotValue(snapshot, "Last NTP sync"));
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

void WifiService::handleApiDeviceInfo() {
  const String snapshot = snapshotHandler_ ? snapshotHandler_(controlContext_) : String("");
  String json;
  json.reserve(960);
  json += F("{\"project\":\"Iris\",\"firmware\":\"");
  json += escapeJson(snapshotValue(snapshot, "Firmware"));
  json += F("\",\"hardware\":\"M5Stack StopWatch\",\"mcu\":\"");
  json += escapeJson(String(ESP.getChipModel()));
  json += F("\",\"mac\":\"");
  json += escapeJson(WiFi.macAddress());
  json += F("\",\"screen\":\"");
  json += escapeJson(snapshotValue(snapshot, "Screen"));
  json += F("\",\"app\":\"");
  json += escapeJson(snapshotValue(snapshot, "App"));
  json += F("\",\"appKind\":\"");
  json += escapeJson(snapshotValue(snapshot, "App kind"));
  json += F("\",\"appState\":\"");
  json += escapeJson(snapshotValue(snapshot, "App state"));
  json += F("\",\"registeredApps\":");
  json += snapshotInt(snapshot, "Registered apps", 0);
  json += F(",\"uptime\":\"");
  json += formatUptime(millis());
  json += F("\",\"cpu\":\"");
  json += escapeJson(snapshotValue(snapshot, "CPU"));
  json += F("\",\"freeHeapBytes\":");
  json += String(ESP.getFreeHeap());
  json += F(",\"freeHeap\":\"");
  json += formatBytes(ESP.getFreeHeap());
  json += F("\",\"flashBytes\":");
  json += String(ESP.getFlashChipSize());
  json += F(",\"flash\":\"");
  json += formatBytes(ESP.getFlashChipSize());
  json += F("\",\"psramBytes\":");
  json += String(ESP.getPsramSize());
  json += F(",\"psram\":\"");
  json += formatBytes(ESP.getPsramSize());
  json += F("\",\"battery\":\"");
  json += escapeJson(snapshotValue(snapshot, "Battery"));
  json += F("\",\"displayPower\":\"");
  json += escapeJson(snapshotValue(snapshot, "Display power"));
  json += F("\",\"powerProfile\":\"");
  json += escapeJson(snapshotValue(snapshot, "Power profile"));
  json += F("\",\"wifi\":\"");
  json += escapeJson(snapshotValue(snapshot, "WiFi"));
  json += F("\",\"ssid\":\"");
  json += escapeJson(snapshotValue(snapshot, "SSID"));
  json += F("\",\"ip\":\"");
  json += escapeJson(snapshotValue(snapshot, "IP"));
  json += F("\",\"services\":\"");
  json += escapeJson(snapshotValue(snapshot, "Services"));
  json += F("\",\"events\":\"");
  json += escapeJson(snapshotValue(snapshot, "Events"));
  json += F("\"}");
  server_.send(200, "application/json", json);
}

void WifiService::handleApiApps() {
  const String snapshot = snapshotHandler_ ? snapshotHandler_(controlContext_) : String("");
  const String registry = snapshotValue(snapshot, "App registry");
  String json;
  json.reserve(1200);
  json += F("{\"current\":\"");
  json += escapeJson(snapshotValue(snapshot, "App"));
  json += F("\",\"kind\":\"");
  json += escapeJson(snapshotValue(snapshot, "App kind"));
  json += F("\",\"state\":\"");
  json += escapeJson(snapshotValue(snapshot, "App state"));
  json += F("\",\"count\":");
  json += String(snapshotInt(snapshot, "Registered apps", 0));
  json += F(",\"apps\":[");
  int start = 0;
  bool first = true;
  while (start < registry.length()) {
    int end = registry.indexOf(';', start);
    if (end < 0) end = registry.length();
    const String item = registry.substring(start, end);
    const int p1 = item.indexOf('|');
    const int p2 = item.indexOf('|', p1 + 1);
    const int p3 = item.indexOf('|', p2 + 1);
    const int p4 = item.indexOf('|', p3 + 1);
    if (p1 > 0 && p2 > p1 && p3 > p2 && p4 > p3) {
      if (!first) json += F(",");
      first = false;
      json += F("{\"id\":\"");
      json += escapeJson(item.substring(0, p1));
      json += F("\",\"name\":\"");
      json += escapeJson(item.substring(p1 + 1, p2));
      json += F("\",\"kind\":\"");
      json += escapeJson(item.substring(p2 + 1, p3));
      json += F("\",\"visibility\":\"");
      json += escapeJson(item.substring(p3 + 1, p4));
      json += F("\",\"state\":\"");
      json += escapeJson(item.substring(p4 + 1));
      json += F("\"}");
    }
    start = end + 1;
  }
  json += F("]}");
  server_.send(200, "application/json", json);
}

void WifiService::handleApiTimeSettings() {
  if (server_.method() == HTTP_PUT) {
    bool boolValue = false;
    const String snapshot = snapshotHandler_ ? snapshotHandler_(controlContext_) : String("");
    if (apiBoolArg("nextCountry", &boolValue) && boolValue) {
      dispatchControlCommand("country_next");
    }
    if (apiBoolArg("nextTimeZone", &boolValue) && boolValue) {
      dispatchControlCommand("timezone_next");
    }
    if (apiBoolArg("nextDateFormat", &boolValue) && boolValue) {
      dispatchControlCommand("date_format_next");
    }
    if (apiBoolArg("nextTimeFormat", &boolValue) && boolValue) {
      dispatchControlCommand("time_format_next");
    }
    if (apiBoolArg("automaticTime", &boolValue) &&
        boolValue != snapshotOn(snapshot, "Automatic time")) {
      dispatchControlCommand("auto_time_toggle");
    }
    if (apiBoolArg("syncNow", &boolValue) && boolValue) {
      dispatchControlCommand("time_sync_now");
    }
    String textValue;
    if (apiStringArg("manualDateTime", &textValue)) {
      dispatchControlCommand(String("manual_time_set:") + textValue);
    }
  }

  const String snapshot = snapshotHandler_ ? snapshotHandler_(controlContext_) : String("");
  String json;
  json.reserve(520);
  json += F("{\"date\":\"");
  json += escapeJson(snapshotValue(snapshot, "Date"));
  json += F("\",\"time\":\"");
  json += escapeJson(snapshotValue(snapshot, "Time"));
  json += F("\",\"country\":\"");
  json += escapeJson(snapshotValue(snapshot, "Country"));
  json += F("\",\"locale\":\"");
  json += escapeJson(snapshotValue(snapshot, "Locale"));
  json += F("\",\"dateFormat\":\"");
  json += escapeJson(snapshotValue(snapshot, "Date format"));
  json += F("\",\"timeFormat\":\"");
  json += escapeJson(snapshotValue(snapshot, "Time format"));
  json += F("\",\"timeZone\":\"");
  json += escapeJson(snapshotValue(snapshot, "Time zone"));
  json += F("\",\"automaticTime\":");
  json += snapshotOn(snapshot, "Automatic time") ? F("true") : F("false");
  json += F(",\"manualDateTimeAllowed\":");
  json += snapshotOn(snapshot, "Automatic time") ? F("false") : F("true");
  json += F(",\"timeSync\":\"");
  json += escapeJson(snapshotValue(snapshot, "Time sync"));
  json += F("\",\"lastNtpSync\":\"");
  json += escapeJson(snapshotValue(snapshot, "Last NTP sync"));
  json += F("\"}");
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
  const String theme = snapshotValue(snapshot, "Theme");
  const int themeId = themeIdForName(theme);
  String json;
  json.reserve(620);
  json += F("{\"theme\":\"");
  json += escapeJson(theme);
  json += F("\",\"themeId\":");
  json += String(themeId);
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
  json += F("\",\"themes\":[");
  for (size_t i = 0; i < sizeof(kWebThemeOptions) / sizeof(kWebThemeOptions[0]); ++i) {
    const WebThemeOption& option = kWebThemeOptions[i];
    if (i > 0) json += F(",");
    json += F("{\"id\":");
    json += String(i);
    json += F(",\"name\":\"");
    json += option.name;
    json += F("\",\"layout\":\"");
    json += option.layout;
    json += F("\",\"background\":\"");
    json += option.background;
    json += F("\",\"foreground\":\"");
    json += option.foreground;
    json += F("\",\"muted\":\"");
    json += option.muted;
    json += F("\",\"accent\":\"");
    json += option.accent;
    json += F("\",\"panel\":\"");
    json += option.panel;
    json += F("\"}");
  }
  json += F("]}");
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
  if (command == "bootloader_confirmed") {
    sendApiOk("Entering bootloader. Iris will disconnect.");
    delay(200);
    dispatchControlCommand(command);
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
  json.reserve(320);
  json += F("{\"enabled\":");
  json += isEnabled() ? F("true") : F("false");
  json += F(",\"connected\":");
  json += isConnected() ? F("true") : F("false");
  json += F(",\"provisioning\":");
  json += isProvisioning() ? F("true") : F("false");
  json += F(",\"hasSavedNetwork\":");
  json += hasSavedNetwork() ? F("true") : F("false");
  json += F(",\"status\":\"");
  json += escapeJson(snapshotValue(snapshot, "WiFi"));
  json += F("\",\"ssid\":\"");
  json += escapeJson(snapshotValue(snapshot, "SSID"));
  json += F("\",\"savedSsid\":\"");
  json += escapeJson(savedSsid_);
  json += F("\",\"ip\":\"");
  json += escapeJson(snapshotValue(snapshot, "IP"));
  json += F("\"}");
  server_.send(200, "application/json", json);
}

void WifiService::handleApiWifiNetworks() {
  String json;
  json.reserve(1200);
  json += F("{\"enabled\":");
  json += isEnabled() ? F("true") : F("false");
  json += F(",\"connected\":");
  json += isConnected() ? F("true") : F("false");
  json += F(",\"provisioning\":");
  json += isProvisioning() ? F("true") : F("false");

  if (!isEnabled()) {
    json += F(",\"scanAvailable\":false,\"message\":\"WiFi is disabled\",\"networks\":[]}");
    server_.send(200, "application/json", json);
    return;
  }

  const int count = WiFi.scanNetworks(false, true);
  json += F(",\"scanAvailable\":true,\"count\":");
  json += String(count > 0 ? count : 0);
  json += F(",\"networks\":[");
  for (int i = 0; i < count; ++i) {
    if (i > 0) json += F(",");
    json += F("{\"ssid\":\"");
    json += escapeJson(WiFi.SSID(i));
    json += F("\",\"rssi\":");
    json += String(WiFi.RSSI(i));
    json += F(",\"channel\":");
    json += String(WiFi.channel(i));
    json += F(",\"encryption\":\"");
    json += encryptionName(WiFi.encryptionType(i));
    json += F("\",\"open\":");
    json += WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? F("true") : F("false");
    json += F("}");
  }
  WiFi.scanDelete();
  json += F("]}");
  server_.send(200, "application/json", json);
}

void WifiService::handleApiWifiConnect() {
  if (!hasSavedNetwork()) {
    sendApiError(409, "No saved WiFi network.");
    return;
  }

  enabled_ = true;
  sendApiOk("Connecting to saved WiFi network.");
  delay(150);
  connectSaved();
}

void WifiService::handleApiWifiDisconnect() {
  if (!enabled_) {
    sendApiOk("WiFi is already disabled.");
    return;
  }

  sendApiOk("Disconnecting from WiFi.");
  delay(150);
  disconnectStation();
}

void WifiService::handleApiWifiReconnect() {
  if (!hasSavedNetwork()) {
    sendApiError(409, "No saved WiFi network.");
    return;
  }

  enabled_ = true;
  sendApiOk("Reconnecting to saved WiFi network.");
  delay(150);
  disconnectStation();
  connectSaved();
}

void WifiService::handleApiWifiForget() {
  if (!hasSavedNetwork()) {
    sendApiOk("No saved WiFi network to forget.");
    return;
  }

  sendApiOk("Saved WiFi credentials removed.");
  delay(150);
  forgetSavedNetwork();
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
  html += F("<title>Iris</title><style>");
  html += F(":root{color-scheme:dark}body{font-family:system-ui,-apple-system,Segoe UI,sans-serif;background:#080908;color:#f5f7f2;margin:0}a{color:inherit}.wrap{max-width:1080px;margin:0 auto;padding:20px}.top{display:grid;grid-template-columns:220px 1fr;gap:22px;align-items:center}.watch{width:190px;height:190px;border-radius:50%;border:8px solid #202420;display:grid;place-items:center;box-shadow:0 0 0 1px #3b433b,0 16px 36px #0008;overflow:hidden}.face{box-sizing:border-box;width:100%;height:100%;padding:28px 18px;text-align:center;display:flex;flex-direction:column;justify-content:center}.time{font-size:42px;font-weight:800;line-height:1}.preview-date{margin-top:10px;color:var(--muted);font-size:15px}.preview-row{display:flex;justify-content:center;gap:6px;flex-wrap:wrap;margin-top:12px}.chip{display:inline-block;border:1px solid var(--panel);border-radius:999px;padding:4px 8px;color:var(--muted);font-size:12px}.preview-meta{margin-top:9px;color:var(--accent);font-size:12px}.title h1{margin:0;font-size:36px}.title p{color:#aab5aa;max-width:680px}.status{display:inline-block;min-height:20px;margin-top:6px;color:#d9f99d;font-size:14px}.layout{display:grid;grid-template-columns:210px minmax(0,1fr);gap:22px;margin-top:22px}.desktop-nav{display:flex;flex-direction:column;gap:8px}.mobile-nav{display:none}.mobile-nav summary{padding:12px 14px;border:1px solid #3a453a;border-radius:8px;background:#111611;color:#fff;cursor:pointer;font-weight:800}.mobile-nav[open] summary{border-color:#d9f99d}.mobile-nav .mobile-links{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:8px;margin-top:8px}.nav{padding:12px 14px;border:1px solid #283028;border-radius:8px;text-decoration:none;background:#111611;color:#d8e2d8}.nav.active{background:#d9f99d;color:#111;border-color:#d9f99d;font-weight:800}section{background:#101410;border:1px solid #283028;border-radius:8px;padding:18px}h2{margin:0 0 16px}h3{margin:20px 0 10px}.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}.grid.three{grid-template-columns:repeat(3,minmax(0,1fr))}.button,button{display:block;box-sizing:border-box;width:100%;padding:12px 13px;border-radius:8px;border:1px solid #3a453a;background:#1a211a;color:#fff;text-align:center;text-decoration:none;font-size:15px;cursor:pointer}.button.warn,button.warn{border-color:#f0c36a;background:#342710}.button.busy,button.busy{opacity:.7}.button.saved,button.saved{border-color:#d9f99d}button:disabled{opacity:.45;cursor:not-allowed}.control{border:1px solid #283028;border-radius:8px;padding:14px;margin:12px 0;background:#0b0e0b}.control.dirty{border-color:#f0c36a;background:#111008}.control label{display:flex;justify-content:space-between;gap:10px;font-weight:700}.control input[type=range]{width:100%;margin:14px 0}.control input[type=number],.control input[type=datetime-local]{background:#050605;color:#fff;border:1px solid #3a453a;border-radius:8px;padding:9px}.control input[type=number]{width:82px}.control form{display:grid;grid-template-columns:1fr auto auto auto;gap:10px;align-items:center}.dirty-message{display:none;grid-column:1/-1;color:#f0c36a;margin:0;font-size:13px}.control.dirty .dirty-message{display:block}.app-list{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}.app-card{border:1px solid #283028;border-radius:8px;background:#0b0e0b;padding:12px}.app-card b,.app-card span,.app-card small{display:block;overflow-wrap:anywhere}.app-card span{color:#aab5aa;margin-top:5px}.app-card small{color:#9faf9f;margin-top:8px}.theme-list{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px;margin-bottom:12px}.theme-card{display:grid;grid-template-columns:76px minmax(0,1fr);gap:10px;align-items:center;border:1px solid #283028;border-radius:8px;background:#0b0e0b;padding:10px;text-decoration:none}.theme-card.active{border-color:#d9f99d;background:#182318}.theme-card b{display:block}.theme-card small{display:block;color:#aab5aa;margin-top:4px}.theme-swatch{height:56px;border:2px solid;border-radius:50%;display:flex;align-items:flex-end;justify-content:center;gap:4px;padding:8px;box-sizing:border-box}.theme-swatch span{width:13px;height:13px;border-radius:50%;border:1px solid #fff6}.network-list{display:grid;gap:8px;margin:12px 0}.network{display:grid;grid-template-columns:minmax(0,1fr) auto;gap:8px;align-items:center;border:1px solid #283028;border-radius:8px;background:#070a07;padding:11px}.network b{overflow-wrap:anywhere}.network span{color:#aab5aa;font-size:13px}.facts{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px;margin-bottom:14px}.facts p{margin:0;padding:12px;border:1px solid #283028;border-radius:8px;background:#0b0e0b}.facts b{display:block;color:#9faf9f;font-size:12px;text-transform:uppercase}.facts span{display:block;margin-top:6px;font-size:18px;overflow-wrap:anywhere}pre{white-space:pre-wrap;background:#050605;border:1px solid #283028;border-radius:8px;padding:12px;color:#cfd8cf}.hint{color:#aab5aa;font-size:14px}.on{border-color:#9ee493;background:#18321d}@media(max-width:760px){.top,.layout{grid-template-columns:1fr}.watch{margin:auto}.desktop-nav{display:none}.mobile-nav{display:block}.grid,.grid.three,.facts,.theme-list,.app-list{grid-template-columns:1fr}.control form{grid-template-columns:1fr}.network{grid-template-columns:1fr}}</style></head><body><div class='wrap'>");
  html += F("<div class='top'>");
  appendWatchPreview(html, snapshot);
  html += F("<div class='title'><h1>Iris</h1><p>Dashboard, device information, settings, and development tools for the M5Stack StopWatch.</p><span id='status' class='status'></span></div></div><div class='layout'>");
  appendNavigation(html, page);
  html += F("<main>");
}

void WifiService::appendPageShellEnd(String& html) {
  html += F("</main></div></div><script>");
  html += F("const statusEl=document.getElementById('status');function note(t){if(statusEl)statusEl.textContent=t||''}");
  html += F("async function sendJson(url,method,payload){const r=await fetch(url,{method,headers:{'Content-Type':'application/json'},body:JSON.stringify(payload||{})});if(!r.ok)throw new Error(await r.text());return r.json()}");
  html += F("async function refreshPreview(){try{const s=await fetch('/api/settings').then(r=>r.json());const time=document.querySelector('.time');const date=document.querySelector('.preview-date');const battery=document.querySelector('[data-preview-battery]');const wifi=document.querySelector('[data-preview-wifi]');const meta=document.querySelector('.preview-meta');if(time&&s.time)time.textContent=s.time;if(date)date.textContent=s.date||'';if(battery)battery.textContent=s.battery||'';if(wifi)wifi.textContent=s.wifi||'';if(meta)meta.textContent=[s.theme,s.layout].filter(Boolean).join(' / ')}catch(e){}}");
  html += F("async function loadWifiNetworks(){const list=document.getElementById('wifi-networks');const state=document.getElementById('wifi-scan-status');if(!list)return;state&&(state.textContent='Scanning...');list.innerHTML='<p class=\"hint\">Scanning nearby networks...</p>';try{const s=await fetch('/api/wifi/networks').then(r=>r.json());if(!s.scanAvailable){state&&(state.textContent='Unavailable');list.innerHTML='<p class=\"hint\">'+(s.message||'Scan unavailable')+'</p>';return}state&&(state.textContent=String(s.count||0)+' found');if(!s.networks||!s.networks.length){list.innerHTML='<p class=\"hint\">No networks found.</p>';return}list.innerHTML=s.networks.map(n=>'<div class=\"network\"><div><b>'+String(n.ssid||'(hidden)').replace(/[&<>]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;'}[c]))+'</b><span>'+n.encryption+' / ch '+n.channel+'</span></div><span>'+n.rssi+' dBm</span></div>').join('')}catch(e){state&&(state.textContent='Failed');list.innerHTML='<p class=\"hint\">Could not scan networks.</p>'}}");
  html += F("document.querySelectorAll('form[data-api]').forEach(f=>{const range=f.querySelector('input[type=range]');const number=f.querySelector('input[type=number]');const value=f.querySelector('[data-value]');const control=f.closest('.control');const reset=f.querySelector('[data-reset]');const current=()=>number?number.value:(range?range.value:'');const setDirty=d=>{control&&control.classList.toggle('dirty',d);reset&&(reset.disabled=!d)};const sync=v=>{if(range)range.value=v;if(number)number.value=v;if(value)value.textContent=v};f.dataset.initial=current();setDirty(false);const changed=()=>setDirty(current()!==f.dataset.initial);if(range)range.addEventListener('input',()=>{sync(range.value);changed()});if(number)number.addEventListener('input',()=>{sync(number.value);changed()});reset&&reset.addEventListener('click',()=>{sync(f.dataset.initial);setDirty(false);note('')});f.addEventListener('submit',async e=>{e.preventDefault();const btn=f.querySelector('button[type=submit]');try{btn&&btn.classList.add('busy');note('Applying...');await sendJson(f.dataset.api,'PUT',{[f.dataset.field]:Number(current())});f.dataset.initial=current();setDirty(false);btn&&btn.classList.add('saved');note('Saved');refreshPreview()}catch(err){note('Could not apply setting')}finally{btn&&btn.classList.remove('busy');setTimeout(()=>{btn&&btn.classList.remove('saved');note('')},1800)}})});");
  html += F("const wifiScanButton=document.getElementById('wifi-scan-button');wifiScanButton&&wifiScanButton.addEventListener('click',loadWifiNetworks);if(document.getElementById('wifi-networks'))loadWifiNetworks();");
  html += F("document.querySelectorAll('[data-wifi-action]').forEach(b=>b.addEventListener('click',async()=>{if(b.dataset.confirm&&!confirm(b.dataset.confirm))return;try{b.classList.add('busy');note('Applying WiFi action...');await sendJson(b.dataset.wifiAction,'POST',{});b.classList.add('saved');note('WiFi action sent');setTimeout(()=>location.reload(),900)}catch(e){note('WiFi action could not be applied')}}));");
  html += F("const wifiForgetButton=document.getElementById('wifi-forget-button');wifiForgetButton&&wifiForgetButton.addEventListener('click',async()=>{if(!confirm('Forget the saved WiFi network? Iris may disconnect from this page.'))return;try{wifiForgetButton.classList.add('busy');note('Forgetting saved network...');await sendJson('/api/wifi/forget','POST',{});wifiForgetButton.classList.add('saved');note('Saved network removed');setTimeout(()=>location.reload(),700)}catch(e){note('Saved network removed. Iris may be reconnecting.')}});");
  html += F("document.querySelectorAll('[data-command]').forEach(a=>a.addEventListener('click',async e=>{e.preventDefault();if(a.dataset.confirm&&!confirm(a.dataset.confirm))return;try{a.classList.add('busy');note('Applying...');await sendJson('/api/command','POST',{command:a.dataset.command});a.classList.add('saved');if(a.dataset.command==='bootloader_confirmed'){note('Entering bootloader. Iris will disconnect.');return}note('Saved');setTimeout(()=>location.reload(),350)}catch(err){note(a.dataset.command==='bootloader_confirmed'?'Iris is disconnecting.':'Could not apply command')}finally{a.classList.remove('busy')}}));");
  html += F("setInterval(refreshPreview,15000);</script></body></html>");
}

void WifiService::appendWatchPreview(String& html, const String& snapshot) {
  String timeText = snapshotValue(snapshot, "Time");
  if (timeText.isEmpty()) timeText = "--:--";
  const String theme = snapshotValue(snapshot, "Theme");
  html += F("<div class='watch' style='");
  html += previewStyleForTheme(theme);
  html += F("'><div class='face'><div class='time'>");
  html += escapeHtml(timeText);
  html += F("</div><div class='preview-date'>");
  html += escapeHtml(snapshotValue(snapshot, "Date"));
  html += F("</div><div class='preview-row'><span class='chip' data-preview-battery>");
  html += escapeHtml(snapshotValue(snapshot, "Battery"));
  html += F("</span><span class='chip' data-preview-wifi>");
  html += escapeHtml(snapshotValue(snapshot, "WiFi"));
  html += F("</span></div><div class='preview-meta'>");
  html += escapeHtml(theme);
  html += F(" / ");
  html += escapeHtml(snapshotValue(snapshot, "Face layout"));
  const String complication = snapshotValue(snapshot, "Complication");
  if (!complication.isEmpty() && complication != "Off") {
    html += F(" / ");
    html += escapeHtml(complication);
  }
  html += F("</div></div></div>");
}

void WifiService::appendNavigation(String& html, const String& page) {
  constexpr const char* pages[] = {"dashboard", "settings", "display", "theme", "datetime",
                                   "touch", "sound", "wifi", "power", "device", "apps", "development"};
  constexpr const char* labels[] = {"Dashboard", "Settings", "Display", "Theme", "Date & Time",
                                    "Touch", "Sound", "WiFi", "Power", "Device", "Apps", "Development"};
  html += F("<nav class='desktop-nav'>");
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
  html += F("<details class='mobile-nav'><summary>");
  bool matched = false;
  for (size_t i = 0; i < sizeof(pages) / sizeof(pages[0]); ++i) {
    if (page == pages[i] || (page.isEmpty() && i == 0)) {
      html += labels[i];
      matched = true;
      break;
    }
  }
  if (!matched) html += F("Menu");
  html += F("</summary><div class='mobile-links'>");
  for (size_t i = 0; i < sizeof(pages) / sizeof(pages[0]); ++i) {
    html += F("<a class='nav");
    if (page == pages[i] || (page.isEmpty() && i == 0)) html += F(" active");
    html += F("' href='/?page=");
    html += pages[i];
    html += F("'>");
    html += labels[i];
    html += F("</a>");
  }
  html += F("</div></details>");
}

void WifiService::appendAppRegistry(String& html, const String& registry) {
  html += F("<div class='app-list'>");
  int start = 0;
  bool any = false;
  while (start < registry.length()) {
    int end = registry.indexOf(';', start);
    if (end < 0) end = registry.length();
    const String item = registry.substring(start, end);
    const int p1 = item.indexOf('|');
    const int p2 = item.indexOf('|', p1 + 1);
    const int p3 = item.indexOf('|', p2 + 1);
    const int p4 = item.indexOf('|', p3 + 1);
    if (p1 > 0 && p2 > p1 && p3 > p2 && p4 > p3) {
      any = true;
      html += F("<div class='app-card'><b>");
      html += escapeHtml(item.substring(p1 + 1, p2));
      html += F("</b><span>");
      html += escapeHtml(item.substring(0, p1));
      html += F("</span><small>");
      html += escapeHtml(item.substring(p2 + 1, p3));
      html += F(" / ");
      html += escapeHtml(item.substring(p3 + 1, p4));
      html += F(" / ");
      html += escapeHtml(item.substring(p4 + 1));
      html += F("</small></div>");
    }
    start = end + 1;
  }
  if (!any) html += F("<p class='hint'>No app registry details available.</p>");
  html += F("</div>");
}

void WifiService::appendRangeControl(String& html, const char* label, const char* command,
                                     int value, int minValue, int maxValue, int step,
                                     const char* suffix) {
  const String page = server_.arg("page");
  const char* endpoint = apiEndpointForCommand(command);
  const char* field = apiFieldForCommand(command);
  html += F("<div class='control'><form method='get' action='/control'");
  if (endpoint[0] != '\0' && field[0] != '\0') {
    html += F(" data-api='");
    html += endpoint;
    html += F("' data-field='");
    html += field;
    html += F("'");
  }
  html += F("><input type='hidden' name='cmd' value='");
  html += command;
  html += F("'><input type='hidden' name='page' value='");
  html += escapeHtml(page);
  html += F("'><label>");
  html += label;
  html += F("<span><span data-value>");
  html += String(value);
  html += F("</span>");
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
  html += F("'><button type='submit'>Apply</button><button type='button' data-reset disabled>Cancel</button><p class='dirty-message'>Unsaved changes</p></form></div>");
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
  html += F("' data-command='");
  html += command;
  html += F("'");
  if (strcmp(command, "bootloader_confirmed") == 0) {
    html += F(" data-confirm='Boot Iris into bootloader mode? WiFi and the web configurator will disconnect until Iris is restarted.'");
  }
  html += F(">");
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
