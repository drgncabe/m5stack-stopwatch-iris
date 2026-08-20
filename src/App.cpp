#include "iris/App.h"

#include <M5Unified.h>
#include "iris/AppConfig.h"
#include "iris/Theme.h"

namespace iris {

namespace {
constexpr MenuItem kMainMenuItems[] = {
    {"Watch", ScreenId::Watch},
    {"Settings", ScreenId::Settings},
};

constexpr MenuItem kSettingsMenuItems[] = {
    {"Volume", ScreenId::Volume},
    {"WiFi", ScreenId::Wifi},
    {"Background", ScreenId::Background},
    {"Power", ScreenId::Power},
    {"Device information", ScreenId::DeviceInfo},
    {"Back to watch", ScreenId::Watch},
};
}  // namespace

App::App()
    : watchScreen_(timeService_, battery_, settings_),
      mainMenuScreen_("Iris", kMainMenuItems,
                      sizeof(kMainMenuItems) / sizeof(kMainMenuItems[0]), settings_),
      settingsMenuScreen_("Settings", kSettingsMenuItems,
                          sizeof(kSettingsMenuItems) / sizeof(kSettingsMenuItems[0]), settings_),
      volumeScreen_(settings_),
      wifiScreen_(settings_, wifi_),
      backgroundScreen_(settings_),
      powerScreen_(settings_),
      deviceInfoScreen_(wifi_, timeService_, battery_, settings_) {}

void App::begin() {
  auto cfg = M5.config();
  cfg.internal_rtc = true;
  cfg.internal_spk = true;
  M5.begin(cfg);

  Serial.begin(115200);
  Serial.println("Iris starting...");

  settings_.begin();
  M5.Speaker.setVolume(settings_.volume());
  M5.Display.setBrightness(settings_.activeBrightness());
  wifi_.setControlCallbacks(this, App::handleControlCommand, App::buildControlSnapshot);

  battery_.begin();
  wifi_.begin(settings_.wifiEnabled());
  wifiDemandStartedMs_ = millis();
  timeService_.begin();

  screenManager_.registerScreen(ScreenId::Watch, &watchScreen_);
  screenManager_.registerScreen(ScreenId::MainMenu, &mainMenuScreen_);
  screenManager_.registerScreen(ScreenId::Settings, &settingsMenuScreen_);
  screenManager_.registerScreen(ScreenId::Volume, &volumeScreen_);
  screenManager_.registerScreen(ScreenId::Wifi, &wifiScreen_);
  screenManager_.registerScreen(ScreenId::Background, &backgroundScreen_);
  screenManager_.registerScreen(ScreenId::Power, &powerScreen_);
  screenManager_.registerScreen(ScreenId::DeviceInfo, &deviceInfoScreen_);

  screenManager_.show(ScreenId::Watch);
  lastActivityMs_ = millis();
}

void App::update() {
  // M5Stack's StopWatch button example requires M5.update() in the main loop.
  // Keep the original serial messages intact while also routing presses to Iris.
  M5.update();

  const uint32_t nowMs = millis();
  bool inputHandled = false;

  if (M5.BtnA.wasPressed()) {
    Serial.println("BtnA Pressed");
    if (displayPowerState_ == DisplayPowerState::Sleeping) {
      wakeDisplay(nowMs);
    } else {
      noteActivity(nowMs);
      screenManager_.onButtonA();
    }
    inputHandled = true;
  }

  if (M5.BtnB.wasPressed()) {
    Serial.println("BtnB Pressed");
    if (displayPowerState_ == DisplayPowerState::Sleeping) {
      wakeDisplay(nowMs);
    } else {
      noteActivity(nowMs);
      screenManager_.onButtonB();
    }
    inputHandled = true;
  }

  const auto touch = M5.Touch.getDetail();
  if (touch.isPressed()) {
    if (!touchActive_) {
      touchActive_ = true;
      if (displayPowerState_ == DisplayPowerState::Sleeping) {
        wakeDisplay(nowMs);
      } else {
        noteActivity(nowMs);
        screenManager_.handleTouch(touch.x, touch.y);
      }
      inputHandled = true;
    }
  } else {
    touchActive_ = false;
  }

  battery_.update(nowMs);
  wifi_.update(nowMs);
  timeService_.update(nowMs, wifi_.isConnected());
  updateWifiPower(nowMs);

  if (displayPowerState_ != DisplayPowerState::Sleeping) {
    screenManager_.update(nowMs);
  }

  if (!inputHandled) {
    updateDisplayPower(nowMs);
  }

  delay(5);
}

void App::handleControlCommand(void* context, const String& command) {
  if (!context) return;
  static_cast<App*>(context)->handleControlCommand(command);
}

String App::buildControlSnapshot(void* context) {
  if (!context) return "Iris unavailable";
  return static_cast<App*>(context)->buildControlSnapshot();
}

void App::handleControlCommand(const String& command) {
  const uint32_t nowMs = millis();
  if (displayPowerState_ == DisplayPowerState::Sleeping) {
    wakeDisplay(nowMs);
  } else {
    noteActivity(nowMs);
  }

  if (command == "watch") {
    screenManager_.show(ScreenId::Watch);
  } else if (command == "settings") {
    screenManager_.show(ScreenId::Settings);
  } else if (command == "btn_a") {
    screenManager_.onButtonA();
  } else if (command == "btn_b") {
    screenManager_.onButtonB();
  } else if (command == "vol_down") {
    adjustVolume(-16);
  } else if (command == "vol_up") {
    adjustVolume(16);
  } else if (command == "wifi_toggle") {
    const bool enabled = !wifi_.isEnabled();
    settings_.setWifiEnabled(enabled);
    wifi_.setEnabled(enabled);
    if (enabled) wifiDemandStartedMs_ = nowMs;
  } else if (command == "wifi_setup") {
    settings_.setWifiEnabled(true);
    wifi_.setEnabled(true);
    wifiDemandStartedMs_ = nowMs;
    wifi_.startProvisioning();
  } else if (command == "bg_next") {
    nextBackground();
  }
}

String App::buildControlSnapshot() const {
  String snapshot;
  snapshot.reserve(240);
  snapshot += "Screen: ";
  snapshot += currentScreenName();
  snapshot += "\nBattery: ";
  snapshot += battery_.statusText();
  snapshot += "\nWiFi: ";
  snapshot += wifi_.statusText();
  snapshot += "\nIP: ";
  snapshot += wifi_.ipAddress();
  snapshot += "\nVolume: ";
  snapshot += String((settings_.volume() * 100) / 255);
  snapshot += "%\nBackground: ";
  snapshot += themeName(settings_);
  return snapshot;
}

void App::adjustVolume(int delta) {
  int next = static_cast<int>(settings_.volume()) + delta;
  next = constrain(next, 0, 255);
  settings_.setVolume(static_cast<uint8_t>(next));
  M5.Speaker.setVolume(static_cast<uint8_t>(next));
  if (next > 0) M5.Speaker.tone(2800, 30);
}

void App::nextBackground() {
  settings_.setWatchBackground((settings_.watchBackground() + 1) % 5);
  if (screenManager_.currentId() == ScreenId::Watch) {
    screenManager_.show(ScreenId::Watch);
  }
}

void App::noteActivity(uint32_t nowMs) {
  lastActivityMs_ = nowMs;
  if (displayPowerState_ == DisplayPowerState::Dimmed) {
    displayPowerState_ = DisplayPowerState::Active;
    M5.Display.setBrightness(settings_.activeBrightness());
  }
}

void App::updateDisplayPower(uint32_t nowMs) {
  const uint32_t idleMs = nowMs - lastActivityMs_;

  const uint16_t sleepSeconds = settings_.sleepTimeoutSeconds();
  if (sleepSeconds > 0 &&
      displayPowerState_ != DisplayPowerState::Sleeping &&
      idleMs >= static_cast<uint32_t>(sleepSeconds) * 1000UL) {
    displayPowerState_ = DisplayPowerState::Sleeping;
    M5.Display.sleep();
    return;
  }

  if (displayPowerState_ == DisplayPowerState::Active &&
      idleMs >= static_cast<uint32_t>(settings_.dimTimeoutSeconds()) * 1000UL) {
    displayPowerState_ = DisplayPowerState::Dimmed;
    M5.Display.setBrightness(config::kDimBrightness);
  }
}

void App::updateWifiPower(uint32_t nowMs) {
  if (!settings_.wifiOnDemand() || !wifi_.isEnabled()) return;
  if (wifi_.isProvisioning()) {
    wifiDemandStartedMs_ = nowMs;
    return;
  }

  const bool synced = timeService_.ntpSynchronized();
  const bool expired = nowMs - wifiDemandStartedMs_ >= config::kWifiOnDemandOffMs;
  if (synced || expired) {
    wifi_.setEnabled(false);
  }
}

void App::wakeDisplay(uint32_t nowMs) {
  lastActivityMs_ = nowMs;
  displayPowerState_ = DisplayPowerState::Active;
  M5.Display.wakeup();
  M5.Display.setBrightness(settings_.activeBrightness());
  screenManager_.show(screenManager_.currentId());
}

const char* App::currentScreenName() const {
  switch (screenManager_.currentId()) {
    case ScreenId::Watch: return "Watch";
    case ScreenId::MainMenu: return "Main menu";
    case ScreenId::Settings: return "Settings";
    case ScreenId::Volume: return "Volume";
    case ScreenId::Wifi: return "WiFi";
    case ScreenId::Background: return "Background";
    case ScreenId::Power: return "Power";
    case ScreenId::DeviceInfo: return "Device info";
    default: return "Unknown";
  }
}

}  // namespace iris
