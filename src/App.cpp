#include "iris/App.h"

#include <M5Unified.h>

namespace iris {

namespace {
constexpr MenuItem kMainMenuItems[] = {
    {"Watch", ScreenId::Watch},
    {"Settings", ScreenId::Settings},
};

constexpr MenuItem kSettingsMenuItems[] = {
    {"Volume", ScreenId::Volume},
    {"WiFi", ScreenId::Wifi},
    {"Device information", ScreenId::DeviceInfo},
    {"Back to watch", ScreenId::Watch},
};
}  // namespace

App::App()
    : watchScreen_(timeService_, battery_),
      mainMenuScreen_("Iris", kMainMenuItems,
                      sizeof(kMainMenuItems) / sizeof(kMainMenuItems[0])),
      settingsMenuScreen_("Settings", kSettingsMenuItems,
                          sizeof(kSettingsMenuItems) / sizeof(kSettingsMenuItems[0])),
      volumeScreen_(settings_),
      wifiScreen_(settings_, wifi_),
      deviceInfoScreen_(wifi_, timeService_, battery_) {}

void App::begin() {
  auto cfg = M5.config();
  cfg.internal_rtc = true;
  cfg.internal_spk = true;
  M5.begin(cfg);

  Serial.begin(115200);
  Serial.println("Iris starting...");

  settings_.begin();
  M5.Speaker.setVolume(settings_.volume());

  battery_.begin();
  wifi_.begin(settings_.wifiEnabled());
  timeService_.begin();

  screenManager_.registerScreen(ScreenId::Watch, &watchScreen_);
  screenManager_.registerScreen(ScreenId::MainMenu, &mainMenuScreen_);
  screenManager_.registerScreen(ScreenId::Settings, &settingsMenuScreen_);
  screenManager_.registerScreen(ScreenId::Volume, &volumeScreen_);
  screenManager_.registerScreen(ScreenId::Wifi, &wifiScreen_);
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
  if (touch.wasClicked()) {
    screenManager_.handleTouch(touch.x, touch.y);
  }

  const uint32_t nowMs = millis();
  battery_.update(nowMs);
  wifi_.update(nowMs);
  timeService_.update(nowMs, wifi_.isConnected());
  screenManager_.update(nowMs);

  delay(5);
}

}  // namespace iris
