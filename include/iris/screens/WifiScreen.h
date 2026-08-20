#pragma once

#include "iris/screens/Screen.h"
#include "iris/services/SettingsStore.h"
#include "iris/services/WifiService.h"

namespace iris {

class WifiScreen : public Screen {
 public:
  WifiScreen(SettingsStore& settings, WifiService& wifi)
      : settings_(settings), wifi_(wifi) {}

  void enter() override;
  void update(uint32_t nowMs) override;
  void draw() override;
  void handleTouch(int32_t x, int32_t y) override;
  void onButtonA() override;
  void onButtonB() override;

 private:
  void toggleWifi();
  void startSetup();
  void goBack();

  SettingsStore& settings_;
  WifiService& wifi_;
  uint32_t lastDrawMs_ = 0;
};

}  // namespace iris
