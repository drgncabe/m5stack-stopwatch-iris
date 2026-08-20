#include "iris/App.h"

#include <M5Unified.h>
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
  wifi_.setControlCallbacks(this, App::handleControlCommand, App::buildControlSnapshot);

  battery_.begin();
  wifi_.begin(settings_.wifiEnabled());
  timeService_.begin();

  screenManager_.registerScreen(ScreenId::Watch, &watchScreen_);
  screenManager_.registerScreen(ScreenId::MainMenu, &mainMenuScreen_);
  screenManager_.registerScreen(ScreenId::Settings, &settingsMenuScreen_);
  screenManager_.registerScreen(ScreenId::Volume, &volumeScreen_);
  screenManager_.registerScreen(ScreenId::Wifi, &wifiScreen_);
  screenManager_.registerScreen(ScreenId::Background, &backgroundScreen_);
  screenManager_.registerScreen(ScreenId::DeviceInfo, &deviceInfoScreen_);

  screenManager_.show(ScreenId::Watch);
}

void App::update() {
  // M5Stack's StopWatch button example requires M5.update() in the main loop.
  // Keep the original serial messages intact while also routing presses to Iris.
  M5.update();

  if (M5.BtnA.wasPressed()) {
    Serial.println("BtnA Pressed");
    screenManager_.onButtonA();
  }

  if (M5.BtnB.wasPressed()) {
    Serial.println("BtnB Pressed");
    screenManager_.onButtonB();
  }

  const auto touch = M5.Touch.getDetail();
  if (touch.isPressed()) {
    if (!touchActive_) {
      touchActive_ = true;
      screenManager_.handleTouch(touch.x, touch.y);
    }
  } else {
    touchActive_ = false;
  }

  const uint32_t nowMs = millis();
  battery_.update(nowMs);
  wifi_.update(nowMs);
  timeService_.update(nowMs, wifi_.isConnected());
  screenManager_.update(nowMs);

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
  } else if (command == "wifi_setup") {
    settings_.setWifiEnabled(true);
    wifi_.setEnabled(true);
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

const char* App::currentScreenName() const {
  switch (screenManager_.currentId()) {
    case ScreenId::Watch: return "Watch";
    case ScreenId::MainMenu: return "Main menu";
    case ScreenId::Settings: return "Settings";
    case ScreenId::Volume: return "Volume";
    case ScreenId::Wifi: return "WiFi";
    case ScreenId::Background: return "Background";
    case ScreenId::DeviceInfo: return "Device info";
    default: return "Unknown";
  }
}

}  // namespace iris
