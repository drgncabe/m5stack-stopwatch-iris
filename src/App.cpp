#include "iris/App.h"

#include <M5Unified.h>
#include "iris/AppConfig.h"
#include "iris/Theme.h"

namespace iris {

namespace {
constexpr uint32_t kMenuReturnTimeoutMs = 30000;
constexpr int32_t kTouchMoveTolerance = 18;
constexpr uint16_t kMinDimSeconds = 5;
constexpr uint16_t kMaxDimSeconds = 120;
constexpr uint8_t kMinDimBrightness = 1;
constexpr uint8_t kMaxDimBrightness = 96;
constexpr uint16_t kMinSleepSeconds = 0;
constexpr uint16_t kMaxSleepSeconds = 600;
constexpr uint16_t kMinTouchDelayMs = 50;
constexpr uint16_t kMaxTouchDelayMs = 500;
constexpr uint16_t kMenuTouchDelayMs = 300;

constexpr MenuItem kMainMenuItems[] = {
    {"Watch", ScreenId::Watch},
    {"Stopwatch", ScreenId::Stopwatch},
    {"Badge", ScreenId::Badge},
    {"Fidgets", ScreenId::Fidgets},
    {"Settings", ScreenId::Settings},
};

constexpr MenuItem kSettingsMenuItems[] = {
    {"Volume", ScreenId::Volume},
    {"WiFi", ScreenId::Wifi},
    {"Date & Time", ScreenId::DateTime},
    {"Theme & widgets", ScreenId::Background},
    {"Power", ScreenId::Power},
    {"Developer", ScreenId::Developer},
    {"Device information", ScreenId::DeviceInfo},
    {"Back to watch", ScreenId::Watch},
};

constexpr MenuItem kDeveloperMenuItems[] = {
    {"HW diagnostics", ScreenId::HardwareDiagnostics},
    {"Axis calibration", ScreenId::AxisCalibration},
    {"Bootloader", ScreenId::Bootloader},
    {"Back", ScreenId::Settings},
};

constexpr MenuItem kFidgetsMenuItems[] = {
    {"Wheel", ScreenId::FidgetWheel},
    {"Poppers", ScreenId::FidgetPoppers},
    {"Kaleidoscope", ScreenId::FidgetSpinner},
    {"Gravity ball", ScreenId::FidgetGravityBall},
    {"Back", ScreenId::MainMenu},
};

bool isFidgetScreen(ScreenId id) {
  return id == ScreenId::FidgetWheel ||
         id == ScreenId::FidgetPoppers ||
         id == ScreenId::FidgetSpinner ||
         id == ScreenId::FidgetGravityBall;
}

bool isStopwatchScreen(ScreenId id) {
  return id == ScreenId::Stopwatch || id == ScreenId::StopwatchLaps;
}

bool isBadgeScreen(ScreenId id) {
  return id == ScreenId::Badge;
}

bool isMenuScreen(ScreenId id) {
  return id == ScreenId::MainMenu ||
         id == ScreenId::Settings ||
         id == ScreenId::Fidgets ||
         id == ScreenId::Developer;
}

constexpr AppDescriptor kAppDefinitions[] = {
    {"system.launcher", "Main menu", ScreenId::MainMenu, AppKind::System, false, nullptr,
     AppUpdateClass::Interactive},
    {"system.fidgets", "Fidgets", ScreenId::Fidgets, AppKind::Fidget, true, nullptr,
     AppUpdateClass::Interactive},
    {"fidget.wheel", "Wheel", ScreenId::FidgetWheel, AppKind::Fidget, false, nullptr,
     AppUpdateClass::Realtime},
    {"fidget.poppers", "Poppers", ScreenId::FidgetPoppers, AppKind::Fidget, false, nullptr,
     AppUpdateClass::Realtime},
    {"fidget.spinner", "Kaleidoscope", ScreenId::FidgetSpinner, AppKind::Fidget, false, nullptr,
     AppUpdateClass::Realtime},
    {"fidget.gravity_ball", "Gravity ball", ScreenId::FidgetGravityBall, AppKind::Fidget, false,
     nullptr, AppUpdateClass::Realtime},
};

constexpr AppDescriptor kSettingsAppDefinitions[] = {
    {"system.settings", "Settings", ScreenId::Settings, AppKind::Settings, true, nullptr,
     AppUpdateClass::Interactive},
    {"settings.volume", "Volume", ScreenId::Volume, AppKind::Settings, false},
    {"settings.wifi", "WiFi", ScreenId::Wifi, AppKind::Settings, false, nullptr,
     AppUpdateClass::Interactive},
    {"settings.date_time", "Date & Time", ScreenId::DateTime, AppKind::Settings, false},
    {"settings.theme", "Theme & widgets", ScreenId::Background, AppKind::Settings, false},
    {"settings.power", "Power", ScreenId::Power, AppKind::Settings, false},
    {"settings.device_info", "Device information", ScreenId::DeviceInfo, AppKind::Settings, false},
};

constexpr AppDescriptor kDevelopmentAppDefinitions[] = {
    {"system.development", "Development", ScreenId::Developer, AppKind::Developer, false, nullptr,
     AppUpdateClass::Interactive},
    {"developer.axis_calibration", "Axis calibration", ScreenId::AxisCalibration,
     AppKind::Developer, false, nullptr, AppUpdateClass::Interactive},
    {"developer.bootloader", "Bootloader", ScreenId::Bootloader, AppKind::Developer, false},
    {"developer.hardware_diagnostics", "Hardware diagnostics", ScreenId::HardwareDiagnostics,
     AppKind::Developer, false, nullptr, AppUpdateClass::Interactive},
};
}  // namespace

App::App()
    : timeService_(settings_),
      power_(settings_),
      appManager_(screenManager_),
      developmentApp_(screenManager_),
      settingsApp_(screenManager_),
      watchScreen_(timeService_, battery_, wifi_, settings_),
      watchApp_(watchScreen_),
      stopwatchScreen_(settings_, stopwatchEngine_),
      stopwatchLapHistoryScreen_(settings_, stopwatchEngine_),
      stopwatchApp_(stopwatchScreen_),
      badgeScreen_(settings_, badge_),
      badgeApp_(badge_, power_),
      mainMenuScreen_("Iris", kMainMenuItems,
                      sizeof(kMainMenuItems) / sizeof(kMainMenuItems[0]), settings_),
      settingsMenuScreen_("Settings", kSettingsMenuItems,
                          sizeof(kSettingsMenuItems) / sizeof(kSettingsMenuItems[0]), settings_),
      volumeScreen_(settings_),
      wifiScreen_(settings_, wifi_),
      dateTimeScreen_(settings_, timeService_, events_),
      backgroundScreen_(settings_),
      powerScreen_(settings_),
      fidgetsMenuScreen_("Fidgets", kFidgetsMenuItems,
                         sizeof(kFidgetsMenuItems) / sizeof(kFidgetsMenuItems[0]), settings_),
      wheelFidgetScreen_(settings_),
      poppersFidgetScreen_(settings_),
      spinnerFidgetScreen_(settings_),
      gravityBallFidgetScreen_(settings_),
      developerMenuScreen_("Developer", kDeveloperMenuItems,
                           sizeof(kDeveloperMenuItems) / sizeof(kDeveloperMenuItems[0]), settings_),
      axisCalibrationScreen_(settings_),
      bootloaderScreen_(settings_),
      hardwareDiagnosticsScreen_(settings_, wifi_, battery_, timeService_, power_, orientation_),
      deviceInfoScreen_(wifi_, timeService_, battery_, settings_) {}

void App::begin() {
  auto cfg = M5.config();
  cfg.internal_rtc = true;
  cfg.internal_spk = true;
  M5.begin(cfg);

  Serial.begin(115200);
  Serial.println("Iris starting...");

  settings_.begin();
  badge_.begin();
  M5.Speaker.setVolume(settings_.volume());
  appManager_.setEventBus(&events_);
  timeService_.setEventBus(&events_);
  power_.begin();
  statusLight_.begin(settings_.indicatorLightEnabled());
  wifi_.setBadgeService(&badge_);
  wifi_.setControlCallbacks(this, App::handleControlCommand, App::buildControlSnapshot);

  battery_.begin();
  orientation_.begin();
  wifi_.begin(settings_.wifiEnabled());
  wifiDemandStartedMs_ = millis();
  timeService_.begin();

  registerServices();
  services_.begin();
  registerScreens();
  registerApps();
  appManager_.begin();

  appManager_.launch("system.watch");
}

void App::registerServices() {
  services_.registerService("events", "Event bus", &events_);
  services_.registerService("badge", "Badge media", &badge_, badge_.mounted());
  services_.registerService("settings", "Settings store", &settings_);
  services_.registerService("battery", "Battery", &battery_);
  services_.registerService("orientation", "Orientation", &orientation_);
  services_.registerService("status_light", "Status light", &statusLight_);
  services_.registerService("power", "Power manager", &power_);
  services_.registerService("wifi", "WiFi", &wifi_, wifi_.isEnabled());
  services_.registerService("time", "Time / RTC", &timeService_);
}

void App::registerScreens() {
  screenManager_.registerScreen(ScreenId::Watch, &watchScreen_);
  screenManager_.registerScreen(ScreenId::Stopwatch, &stopwatchScreen_);
  screenManager_.registerScreen(ScreenId::StopwatchLaps, &stopwatchLapHistoryScreen_);
  screenManager_.registerScreen(ScreenId::Badge, &badgeScreen_);
  screenManager_.registerScreen(ScreenId::MainMenu, &mainMenuScreen_);
  screenManager_.registerScreen(ScreenId::Settings, &settingsMenuScreen_);
  screenManager_.registerScreen(ScreenId::Volume, &volumeScreen_);
  screenManager_.registerScreen(ScreenId::Wifi, &wifiScreen_);
  screenManager_.registerScreen(ScreenId::DateTime, &dateTimeScreen_);
  screenManager_.registerScreen(ScreenId::Background, &backgroundScreen_);
  screenManager_.registerScreen(ScreenId::Power, &powerScreen_);
  screenManager_.registerScreen(ScreenId::Fidgets, &fidgetsMenuScreen_);
  screenManager_.registerScreen(ScreenId::FidgetWheel, &wheelFidgetScreen_);
  screenManager_.registerScreen(ScreenId::FidgetPoppers, &poppersFidgetScreen_);
  screenManager_.registerScreen(ScreenId::FidgetSpinner, &spinnerFidgetScreen_);
  screenManager_.registerScreen(ScreenId::FidgetGravityBall, &gravityBallFidgetScreen_);
  screenManager_.registerScreen(ScreenId::Developer, &developerMenuScreen_);
  screenManager_.registerScreen(ScreenId::AxisCalibration, &axisCalibrationScreen_);
  screenManager_.registerScreen(ScreenId::Bootloader, &bootloaderScreen_);
  screenManager_.registerScreen(ScreenId::HardwareDiagnostics, &hardwareDiagnosticsScreen_);
  screenManager_.registerScreen(ScreenId::DeviceInfo, &deviceInfoScreen_);
}

void App::registerApps() {
  appManager_.registerApp(
      AppDescriptor(watchApp_.id(), watchApp_.name(), ScreenId::Watch, AppKind::System, true,
                    &watchApp_, AppUpdateClass::Normal));
  appManager_.registerApp(AppDescriptor(stopwatchApp_.id(), stopwatchApp_.name(),
                                        ScreenId::Stopwatch, AppKind::Tool, true,
                                        &stopwatchApp_, AppUpdateClass::Realtime));
  appManager_.registerApp(AppDescriptor(badgeApp_.id(), badgeApp_.name(), ScreenId::Badge,
                                        AppKind::Tool, true, &badgeApp_,
                                        AppUpdateClass::Normal));
  for (size_t i = 0; i < sizeof(kSettingsAppDefinitions) / sizeof(kSettingsAppDefinitions[0]);
       ++i) {
    AppDescriptor app = kSettingsAppDefinitions[i];
    app.application = &settingsApp_;
    appManager_.registerApp(app);
  }
  for (size_t i = 0;
       i < sizeof(kDevelopmentAppDefinitions) / sizeof(kDevelopmentAppDefinitions[0]); ++i) {
    AppDescriptor app = kDevelopmentAppDefinitions[i];
    app.application = &developmentApp_;
    appManager_.registerApp(app);
  }
  for (size_t i = 0; i < sizeof(kAppDefinitions) / sizeof(kAppDefinitions[0]); ++i) {
    appManager_.registerApp(kAppDefinitions[i]);
  }
}

void App::update() {
  // M5Stack's StopWatch button example requires M5.update() in the main loop.
  // Keep the original serial messages intact while also routing presses to Iris.
  M5.update();

  const uint32_t nowMs = millis();
  bool inputHandled = false;
  const bool previousWifiConnected = wifi_.isConnected();
  const uint8_t previousRotation = orientation_.rotation();

  if (power_.state() != DisplayPowerState::Sleeping &&
      orientation_.update(nowMs, settings_.autoRotate(), settings_.accelOffsetX(),
                          settings_.accelOffsetY(), settings_.accelOffsetZ())) {
    resetTouch();
    screenManager_.redraw();
  }

  if (M5.BtnA.wasPressed()) {
    Serial.println("BtnA Pressed");
    if (power_.state() == DisplayPowerState::Sleeping) {
      wakeDisplay(nowMs);
    } else {
      noteActivity(nowMs);
      if (!appManager_.onButtonA()) screenManager_.onButtonA();
    }
    inputHandled = true;
  }

  if (M5.BtnB.wasPressed()) {
    Serial.println("BtnB Pressed");
    if (power_.state() == DisplayPowerState::Sleeping) {
      wakeDisplay(nowMs);
    } else {
      noteActivity(nowMs);
      if (!appManager_.onButtonB()) screenManager_.onButtonB();
    }
    inputHandled = true;
  }

  const auto touch = M5.Touch.getDetail();
  if (touch.isPressed()) {
    if (!touchActive_) {
      touchActive_ = true;
      touchPreviewed_ = false;
      touchHandled_ = false;
      touchMoved_ = false;
      touchStartX_ = touch.x;
      touchStartY_ = touch.y;
      touchStartMs_ = nowMs;
      if (power_.state() == DisplayPowerState::Sleeping) {
        wakeDisplay(nowMs);
        touchHandled_ = true;
      } else {
        noteActivity(nowMs);
        screenManager_.previewTouch(touch.x, touch.y);
        touchPreviewed_ = true;
      }
      inputHandled = true;
    } else if (!touchHandled_) {
      const bool movedTooFar = abs(touch.x - touchStartX_) > kTouchMoveTolerance ||
                               abs(touch.y - touchStartY_) > kTouchMoveTolerance;
      const uint16_t configuredDelayMs = settings_.touchDelayMs();
      const uint16_t touchDelayMs =
          isMenuScreen(screenManager_.currentId()) && configuredDelayMs < kMenuTouchDelayMs
              ? kMenuTouchDelayMs
              : configuredDelayMs;
      if (!movedTooFar && !touchMoved_ && nowMs - touchStartMs_ >= touchDelayMs) {
        if (!appManager_.onTouch(touchStartX_, touchStartY_)) {
          screenManager_.handleTouch(touchStartX_, touchStartY_);
        }
        touchHandled_ = true;
        inputHandled = true;
      } else if (movedTooFar && touchPreviewed_) {
        touchMoved_ = true;
        touchStartX_ = touch.x;
        touchStartY_ = touch.y;
        touchStartMs_ = nowMs;
        screenManager_.previewTouch(touch.x, touch.y);
      }
    }
  } else {
    if (touchActive_ && !touchHandled_ && touchPreviewed_ && !touchMoved_) {
      if (!appManager_.onTouch(touchStartX_, touchStartY_)) {
        screenManager_.handleTouch(touchStartX_, touchStartY_);
      }
      inputHandled = true;
    }
    touchActive_ = false;
    touchPreviewed_ = false;
    touchHandled_ = false;
    touchMoved_ = false;
  }

  battery_.update(nowMs);
  wifi_.update(nowMs);
  services_.setStarted("wifi", wifi_.isEnabled());
  timeService_.update(nowMs, wifi_.isConnected());
  updateWifiPower(nowMs);
  services_.update(nowMs);
  updateSystemEvents(previousWifiConnected, previousRotation);

  const AppDescriptor* currentApp = appManager_.current();
  if (shouldUpdateForeground(nowMs, currentApp)) {
    screenManager_.update(nowMs);
    appManager_.update(nowMs);
  }
  appManager_.syncToCurrentScreen();

  if (!inputHandled) {
    updateDisplayPower(nowMs);
  }

  delay(power_.loopDelayMs(currentApp));
}

void App::handleControlCommand(void* context, const String& command) {
  if (!context) return;
  static_cast<App*>(context)->handleControlCommand(command);
}

void App::updateSystemEvents(bool previousWifiConnected, uint8_t previousRotation) {
  const bool wifiConnected = wifi_.isConnected();
  if (wifiConnected != previousWifiConnected) {
    events_.publish(wifiConnected ? EventType::WifiConnected : EventType::WifiDisconnected,
                    "WifiService", wifi_.ssid().c_str());
  }

  if (orientation_.rotation() != previousRotation) {
    events_.publish(EventType::ImuOrientationChanged, "OrientationService", nullptr,
                    orientation_.rotation());
  }

  const BatterySnapshot battery = battery_.snapshot();
  const bool batteryLow = battery.percent >= 0 && battery.percent <= 15 && !battery.charging;
  if (batteryLow && !batteryLowPublished_) {
    events_.publish(EventType::BatteryLow, "BatteryService", nullptr, battery.percent);
  }
  batteryLowPublished_ = batteryLow;
}

String App::buildControlSnapshot(void* context) {
  if (!context) return "Iris unavailable";
  return static_cast<App*>(context)->buildControlSnapshot();
}

void App::handleControlCommand(const String& command) {
  const uint32_t nowMs = millis();
  if (power_.state() == DisplayPowerState::Sleeping) {
    wakeDisplay(nowMs);
  } else {
    noteActivity(nowMs);
  }

  if (command == "watch") {
    appManager_.launch("system.watch");
  } else if (command == "settings") {
    appManager_.launch("system.settings");
  } else if (command == "date_time") {
    appManager_.launch("settings.date_time");
  } else if (command == "badge") {
    appManager_.launch("media.badge");
  } else if (command == "development") {
    appManager_.launch("system.development");
  } else if (command == "hardware_diagnostics") {
    appManager_.launch("developer.hardware_diagnostics");
  } else if (command == "bootloader") {
    appManager_.launch("developer.bootloader");
  } else if (command == "bootloader_confirmed") {
    enterBootloaderFromWeb();
  } else if (command == "btn_a") {
    if (!appManager_.onButtonA()) screenManager_.onButtonA();
  } else if (command == "btn_b") {
    if (!appManager_.onButtonB()) screenManager_.onButtonB();
  } else if (command == "vol_down") {
    adjustVolume(-16);
  } else if (command == "vol_up") {
    adjustVolume(16);
  } else if (command.startsWith("volume_set:")) {
    const int percent = constrain(command.substring(11).toInt(), 0, 100);
    const uint8_t volume = static_cast<uint8_t>((percent * 255) / 100);
    settings_.setVolume(volume);
    M5.Speaker.setVolume(volume);
    if (volume > 0) M5.Speaker.tone(2800, 30);
  } else if (command == "bright_down") {
    adjustBrightness(-16);
  } else if (command == "bright_up") {
    adjustBrightness(16);
  } else if (command.startsWith("brightness_set:")) {
    const int brightness = constrain(command.substring(15).toInt(), 16, 255);
    settings_.setActiveBrightness(static_cast<uint8_t>(brightness));
    if (power_.state() == DisplayPowerState::Active) {
      M5.Display.setBrightness(static_cast<uint8_t>(brightness));
    }
  } else if (command == "dim_down") {
    adjustDimTimeout(-5);
  } else if (command == "dim_up") {
    adjustDimTimeout(5);
  } else if (command.startsWith("dim_brightness_set:")) {
    const int brightness =
        constrain(command.substring(19).toInt(), kMinDimBrightness, kMaxDimBrightness);
    settings_.setDimBrightness(static_cast<uint8_t>(brightness));
    if (power_.state() == DisplayPowerState::Dimmed) {
      M5.Display.setBrightness(static_cast<uint8_t>(brightness));
    }
  } else if (command.startsWith("dim_set:")) {
    const int seconds = constrain(command.substring(8).toInt(), kMinDimSeconds, kMaxDimSeconds);
    settings_.setDimTimeoutSeconds(static_cast<uint16_t>(seconds));
  } else if (command == "sleep_down") {
    adjustSleepTimeout(-15);
  } else if (command == "sleep_up") {
    adjustSleepTimeout(15);
  } else if (command.startsWith("sleep_set:")) {
    const int seconds = constrain(command.substring(10).toInt(), kMinSleepSeconds, kMaxSleepSeconds);
    settings_.setSleepTimeoutSeconds(static_cast<uint16_t>(seconds));
  } else if (command == "touch_down") {
    adjustTouchDelay(-25);
  } else if (command == "touch_up") {
    adjustTouchDelay(25);
  } else if (command.startsWith("touch_set:")) {
    const int delayMs = constrain(command.substring(10).toInt(), kMinTouchDelayMs, kMaxTouchDelayMs);
    settings_.setTouchDelayMs(static_cast<uint16_t>(delayMs));
  } else if (command == "low_face_toggle") {
    settings_.setLowPowerFace(!settings_.lowPowerFace());
    showWatchIfActive();
  } else if (command == "power_profile_next") {
    const uint8_t next =
        (static_cast<uint8_t>(settings_.powerProfile()) + 1) %
        (static_cast<uint8_t>(PowerProfile::Performance) + 1);
    settings_.setPowerProfile(static_cast<PowerProfile>(next));
  } else if (command == "auto_rotate_toggle") {
    settings_.setAutoRotate(!settings_.autoRotate());
  } else if (command == "indicator_toggle") {
    setIndicatorLight(!settings_.indicatorLightEnabled());
  } else if (command == "country_next") {
    settings_.setCountryRegion(static_cast<CountryRegion>(
        (static_cast<uint8_t>(settings_.countryRegion()) + 1) % kCountryRegionCount));
    events_.publish(EventType::LocaleChanged, "App", localeCode(settings_.countryRegion()));
  } else if (command == "timezone_next") {
    settings_.setTimeZone(static_cast<TimeZoneId>(
        (static_cast<uint8_t>(settings_.timeZone()) + 1) % kTimeZoneCount));
    timeService_.applyConfiguredTimezone();
    events_.publish(EventType::TimeZoneChanged, "App", timeZoneIanaName(settings_.timeZone()));
  } else if (command == "date_format_next") {
    settings_.setDateFormat(static_cast<DateFormat>(
        (static_cast<uint8_t>(settings_.dateFormat()) + 1) % kDateFormatCount));
  } else if (command == "time_format_next") {
    settings_.setTimeFormat(static_cast<TimeFormat>(
        (static_cast<uint8_t>(settings_.timeFormat()) + 1) % kTimeFormatCount));
  } else if (command == "auto_time_toggle") {
    settings_.setAutomaticTimeEnabled(!settings_.automaticTimeEnabled());
  } else if (command == "time_sync_now") {
    timeService_.syncNow(nowMs);
  } else if (command.startsWith("manual_time_set:")) {
    timeService_.setManualDateTimeText(command.substring(16));
  } else if (command == "wifi_demand_toggle") {
    settings_.setWifiOnDemand(!settings_.wifiOnDemand());
    if (settings_.wifiOnDemand() && wifi_.isEnabled()) wifiDemandStartedMs_ = nowMs;
  } else if (command == "wifi_toggle") {
    const bool enabled = !wifi_.isEnabled();
    settings_.setWifiEnabled(enabled);
    wifi_.setEnabled(enabled);
    services_.setStarted("wifi", enabled);
    if (enabled) wifiDemandStartedMs_ = nowMs;
  } else if (command == "wifi_setup") {
    settings_.setWifiEnabled(true);
    wifi_.setEnabled(true);
    services_.setStarted("wifi", true);
    wifiDemandStartedMs_ = nowMs;
    wifi_.startProvisioning();
  } else if (command == "bg_next") {
    nextTheme();
  } else if (command.startsWith("theme_")) {
    const int themeId = command.substring(6).toInt();
    settings_.setThemeId(constrain(themeId, 0, kThemeCount - 1));
    showWatchIfActive();
  } else if (command == "widget_battery_toggle") {
    settings_.setWidgetEnabled(kWidgetBattery, !settings_.widgetEnabled(kWidgetBattery));
    showWatchIfActive();
  } else if (command == "widget_date_toggle") {
    settings_.setWidgetEnabled(kWidgetDate, !settings_.widgetEnabled(kWidgetDate));
    showWatchIfActive();
  } else if (command == "widget_seconds_toggle") {
    settings_.setWidgetEnabled(kWidgetSeconds, !settings_.widgetEnabled(kWidgetSeconds));
    showWatchIfActive();
  } else if (command == "widget_wifi_toggle") {
    settings_.setWidgetEnabled(kWidgetWifi, !settings_.widgetEnabled(kWidgetWifi));
    showWatchIfActive();
  } else if (command == "complication_next") {
    nextComplication();
  } else if (command == "badge_mode_next") {
    nextBadgeMode();
  } else if (command == "badge_keep_awake_toggle") {
    badge_.setKeepAwake(!badge_.keepAwake());
    badgeApp_.refreshWakeLock();
  }
}

String App::buildControlSnapshot() const {
  String snapshot;
  snapshot.reserve(1600);
  snapshot += "Screen: ";
  snapshot += currentScreenName();
  snapshot += "\nFirmware: ";
  snapshot += config::kVersion;
  const AppDescriptor* currentApp = appManager_.current();
  snapshot += "\nApp: ";
  snapshot += currentApp ? currentApp->id : "unknown";
  snapshot += "\nApp kind: ";
  snapshot += currentApp ? appKindName(currentApp->kind) : "Unknown";
  snapshot += "\nApp state: ";
  snapshot += appManager_.currentStateName();
  snapshot += "\nApp update: ";
  snapshot += currentApp ? appUpdateClassName(currentApp->updateClass) : "Unknown";
  snapshot += "\nRegistered apps: ";
  snapshot += String(appManager_.count());
  snapshot += "\nApp registry: ";
  for (size_t i = 0; i < appManager_.count(); ++i) {
    const AppDescriptor* app = appManager_.appAt(i);
    if (!app) continue;
    if (i > 0) snapshot += ";";
    snapshot += app->id;
    snapshot += "|";
    snapshot += app->name;
    snapshot += "|";
    snapshot += appKindName(app->kind);
    snapshot += "|";
    snapshot += app->visible ? "Visible" : "Hidden";
    snapshot += "|";
    snapshot += appLifecycleStateName(appManager_.stateAt(i));
    snapshot += "|";
    snapshot += appUpdateClassName(app->updateClass);
  }
  snapshot += "\nServices: ";
  snapshot += services_.summary();
  snapshot += "\nEvents: ";
  snapshot += events_.summary();
  snapshot += "\nWiFi service: ";
  snapshot += services_.stateName("wifi");
  snapshot += "\nDisplay power: ";
  snapshot += power_.stateName();
  snapshot += "\nPower profile: ";
  snapshot += power_.profileName();
  snapshot += "\nPower requests: ";
  snapshot += power_.requestSummary();
  snapshot += "\nCPU: ";
  snapshot += String(power_.currentCpuMhz());
  snapshot += " MHz";
  snapshot += "\nLoop delay: ";
  snapshot += String(power_.loopDelayMs(currentApp));
  snapshot += "ms";
  snapshot += "\nForeground update: ";
  const uint16_t foregroundIntervalMs = power_.foregroundUpdateIntervalMs(currentApp);
  snapshot += foregroundIntervalMs == 0 ? String("Unthrottled") : String(foregroundIntervalMs) + "ms";
  snapshot += "\nBattery: ";
  snapshot += battery_.statusText();
  snapshot += "\nWiFi: ";
  snapshot += wifi_.statusText();
  snapshot += "\nSSID: ";
  snapshot += wifi_.ssid();
  snapshot += "\nIP: ";
  snapshot += wifi_.ipAddress();
  const DateTimeSnapshot time = timeService_.now();
  snapshot += "\nTime: ";
  snapshot += timeService_.formatTime(time);
  snapshot += "\nDate: ";
  snapshot += timeService_.formatDate(time);
  snapshot += "\nCountry: ";
  snapshot += countryRegionName(settings_.countryRegion());
  snapshot += "\nLocale: ";
  snapshot += localeCode(settings_.countryRegion());
  snapshot += "\nDate format: ";
  snapshot += dateFormatName(settings_.dateFormat());
  snapshot += "\nTime format: ";
  snapshot += timeFormatName(settings_.timeFormat());
  snapshot += "\nTime zone: ";
  snapshot += timeZoneIanaName(settings_.timeZone());
  snapshot += "\nAutomatic time: ";
  snapshot += settings_.automaticTimeEnabled() ? "On" : "Off";
  snapshot += "\nTime sync: ";
  snapshot += timeService_.syncStatusText();
  snapshot += "\nLast NTP sync: ";
  snapshot += timeService_.lastNtpSyncText();
  snapshot += "\nVolume: ";
  snapshot += String((settings_.volume() * 100) / 255);
  snapshot += "%\nTheme: ";
  snapshot += themeName(settings_);
  snapshot += "\nFace layout: ";
  snapshot += watchLayoutName(currentTheme(settings_).layout);
  snapshot += "\nWidgets: ";
  const bool hasFaceWidget = settings_.widgetEnabled(kWidgetBattery) ||
                             settings_.widgetEnabled(kWidgetDate) ||
                             settings_.widgetEnabled(kWidgetSeconds) ||
                             settings_.widgetEnabled(kWidgetWifi);
  snapshot += settings_.widgetEnabled(kWidgetBattery) ? "Battery " : "";
  snapshot += settings_.widgetEnabled(kWidgetDate) ? "Date " : "";
  snapshot += settings_.widgetEnabled(kWidgetSeconds) ? "Seconds " : "";
  snapshot += settings_.widgetEnabled(kWidgetWifi) ? "WiFi" : "";
  if (!hasFaceWidget) snapshot += "None";
  snapshot += "\nComplication: ";
  snapshot += settings_.widgetEnabled(kWidgetComplication) ? complicationName(settings_.complicationId()) : "Off";
  snapshot += "\nBrightness: ";
  snapshot += String(settings_.activeBrightness());
  snapshot += "/255";
  snapshot += "\nDim brightness: ";
  snapshot += String(settings_.dimBrightness());
  snapshot += "/255";
  snapshot += "\nDim timeout: ";
  snapshot += String(settings_.dimTimeoutSeconds());
  snapshot += "s";
  snapshot += "\nSleep timeout: ";
  snapshot += settings_.sleepTimeoutSeconds() == 0 ? String("Off") : String(settings_.sleepTimeoutSeconds()) + "s";
  snapshot += "\nLow-power face: ";
  snapshot += settings_.lowPowerFace() ? "On" : "Off";
  snapshot += "\nAuto rotate: ";
  snapshot += settings_.autoRotate() ? orientation_.statusText() : "Off";
  snapshot += " r";
  snapshot += String(orientation_.rotation());
  snapshot += "\nIndicator light: ";
  snapshot += settings_.indicatorLightEnabled() ? "On" : "Off";
  snapshot += "\nWiFi on demand: ";
  snapshot += settings_.wifiOnDemand() ? "On" : "Off";
  snapshot += "\nTouch delay: ";
  snapshot += String(settings_.touchDelayMs());
  snapshot += "ms";
  snapshot += "\nBadge: ";
  snapshot += badge_.statusText();
  snapshot += "\nBadge type: ";
  snapshot += badge_.typeName();
  snapshot += "\nBadge mode: ";
  snapshot += badge_.modeName();
  snapshot += "\nBadge awake: ";
  snapshot += badge_.keepAwake() ? "On" : "Off";
  return snapshot;
}

void App::adjustVolume(int delta) {
  int next = static_cast<int>(settings_.volume()) + delta;
  next = constrain(next, 0, 255);
  settings_.setVolume(static_cast<uint8_t>(next));
  M5.Speaker.setVolume(static_cast<uint8_t>(next));
  if (next > 0) M5.Speaker.tone(2800, 30);
}

void App::adjustBrightness(int delta) {
  int next = static_cast<int>(settings_.activeBrightness()) + delta;
  next = constrain(next, 16, 255);
  settings_.setActiveBrightness(static_cast<uint8_t>(next));
  if (power_.state() == DisplayPowerState::Active) {
    M5.Display.setBrightness(static_cast<uint8_t>(next));
  }
}

void App::adjustDimTimeout(int delta) {
  int next = static_cast<int>(settings_.dimTimeoutSeconds()) + delta;
  next = constrain(next, kMinDimSeconds, kMaxDimSeconds);
  settings_.setDimTimeoutSeconds(static_cast<uint16_t>(next));
}

void App::adjustSleepTimeout(int delta) {
  int next = static_cast<int>(settings_.sleepTimeoutSeconds()) + delta;
  next = constrain(next, kMinSleepSeconds, kMaxSleepSeconds);
  settings_.setSleepTimeoutSeconds(static_cast<uint16_t>(next));
}

void App::adjustTouchDelay(int delta) {
  int next = static_cast<int>(settings_.touchDelayMs()) + delta;
  next = constrain(next, kMinTouchDelayMs, kMaxTouchDelayMs);
  settings_.setTouchDelayMs(static_cast<uint16_t>(next));
}

void App::setIndicatorLight(bool enabled) {
  settings_.setIndicatorLightEnabled(enabled);
  statusLight_.setEnabled(enabled);
}

void App::enterBootloaderFromWeb() {
  Serial.println("[BOOT] Web configurator requested ROM download mode");
  appManager_.launch("developer.bootloader");
  statusLight_.clearNotification();
  wifi_.shutdownForBootloader();
  Serial.println("[BOOT] Entering ESP32-S3 bootloader");
  Serial.flush();
  bootloaderScreen_.requestBootloader();
}

void App::nextTheme() {
  settings_.setThemeId((settings_.themeId() + 1) % kThemeCount);
  showWatchIfActive();
}

void App::nextComplication() {
  settings_.setComplicationId((settings_.complicationId() + 1) % kComplicationCount);
  showWatchIfActive();
}

void App::nextBadgeMode() {
  badge_.nextMode();
  if (screenManager_.currentId() == ScreenId::Badge) {
    screenManager_.show(ScreenId::Badge);
  }
}

void App::resetTouch() {
  touchActive_ = false;
  touchPreviewed_ = false;
  touchHandled_ = false;
  touchMoved_ = false;
}

void App::showWatchIfActive() {
  if (screenManager_.currentId() == ScreenId::Watch) {
    screenManager_.show(ScreenId::Watch);
  }
}

void App::noteActivity(uint32_t nowMs) {
  power_.userActivity(nowMs);
}

void App::updateDisplayPower(uint32_t nowMs) {
  const uint32_t idleMs = power_.idleMs(nowMs);

  const ScreenId current = screenManager_.currentId();
  if (current != ScreenId::Watch && !isFidgetScreen(current) && !isStopwatchScreen(current) &&
      !isBadgeScreen(current) &&
      idleMs >= kMenuReturnTimeoutMs) {
    appManager_.launch("system.watch");
    power_.userActivity(nowMs);
    return;
  }

  power_.update(nowMs, appManager_.current());
}

void App::updateWifiPower(uint32_t nowMs) {
  if (!settings_.wifiOnDemand() || !wifi_.isEnabled()) return;
  if (power_.wifiRequested()) {
    wifiDemandStartedMs_ = nowMs;
    return;
  }
  if (wifi_.isProvisioning()) {
    wifiDemandStartedMs_ = nowMs;
    return;
  }

  const bool synced = timeService_.ntpSynchronized();
  const bool expired = nowMs - wifiDemandStartedMs_ >= config::kWifiOnDemandOffMs;
  if (synced || expired) {
    wifi_.setEnabled(false);
    services_.setStarted("wifi", false);
  }
}

void App::wakeDisplay(uint32_t nowMs) {
  power_.wake(nowMs);
  lastForegroundUpdateMs_ = 0;
  appManager_.switchTo(screenManager_.currentId());
}

bool App::shouldUpdateForeground(uint32_t nowMs, const AppDescriptor* app) {
  const uint16_t intervalMs = power_.foregroundUpdateIntervalMs(app);
  if (power_.state() == DisplayPowerState::Sleeping) return false;
  if (intervalMs == 0 || lastForegroundUpdateMs_ == 0 ||
      nowMs - lastForegroundUpdateMs_ >= intervalMs) {
    lastForegroundUpdateMs_ = nowMs;
    return true;
  }
  return false;
}

const char* App::currentScreenName() const {
  switch (screenManager_.currentId()) {
    case ScreenId::Watch: return "Watch";
    case ScreenId::Stopwatch: return "Stopwatch";
    case ScreenId::StopwatchLaps: return "Stopwatch laps";
    case ScreenId::Badge: return "Badge";
    case ScreenId::MainMenu: return "Main menu";
    case ScreenId::Settings: return "Settings";
    case ScreenId::Volume: return "Volume";
    case ScreenId::Wifi: return "WiFi";
    case ScreenId::DateTime: return "Date & Time";
    case ScreenId::Background: return "Theme & widgets";
    case ScreenId::Power: return "Power";
    case ScreenId::Fidgets: return "Fidgets";
    case ScreenId::FidgetWheel: return "Fidget wheel";
    case ScreenId::FidgetPoppers: return "Fidget poppers";
    case ScreenId::FidgetSpinner: return "Fidget kaleidoscope";
    case ScreenId::FidgetGravityBall: return "Fidget gravity ball";
    case ScreenId::Developer: return "Developer";
    case ScreenId::AxisCalibration: return "Axis calibration";
    case ScreenId::Bootloader: return "Bootloader";
    case ScreenId::HardwareDiagnostics: return "Hardware diagnostics";
    case ScreenId::DeviceInfo: return "Device info";
    default: return "Unknown";
  }
}

}  // namespace iris
