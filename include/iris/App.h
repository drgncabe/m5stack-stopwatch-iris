#pragma once

#include "iris/screens/DeviceInfoScreen.h"
#include "iris/screens/MenuScreen.h"
#include "iris/screens/ScreenManager.h"
#include "iris/screens/VolumeScreen.h"
#include "iris/screens/WatchScreen.h"
#include "iris/screens/WifiScreen.h"
#include "iris/services/SettingsStore.h"
#include "iris/services/TimeService.h"
#include "iris/services/WifiService.h"

namespace iris {

class App {
 public:
  App();

  void begin();
  void update();

 private:
  SettingsStore settings_;
  WifiService wifi_;
  TimeService timeService_;
  ScreenManager screenManager_;

  WatchScreen watchScreen_;
  MenuScreen mainMenuScreen_;
  MenuScreen settingsMenuScreen_;
  VolumeScreen volumeScreen_;
  WifiScreen wifiScreen_;
  DeviceInfoScreen deviceInfoScreen_;
};

}  // namespace iris
