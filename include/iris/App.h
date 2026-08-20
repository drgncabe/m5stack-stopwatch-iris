#pragma once

#include "iris/screens/BackgroundScreen.h"
#include "iris/screens/DeviceInfoScreen.h"
#include "iris/screens/MenuScreen.h"
#include "iris/screens/PowerScreen.h"
#include "iris/screens/ScreenManager.h"
#include "iris/screens/VolumeScreen.h"
#include "iris/screens/WatchScreen.h"
#include "iris/screens/WifiScreen.h"
#include "iris/services/BatteryService.h"
#include "iris/services/SettingsStore.h"
#include "iris/services/TimeService.h"
#include "iris/services/WifiService.h"

namespace iris {

enum class DisplayPowerState : uint8_t {
  Active,
  Dimmed,
  Sleeping,
};

class App {
 public:
  App();

  void begin();
  void update();

 private:
  static void handleControlCommand(void* context, const String& command);
  static String buildControlSnapshot(void* context);

  void handleControlCommand(const String& command);
  String buildControlSnapshot() const;
  void adjustVolume(int delta);
  void nextBackground();
  void noteActivity(uint32_t nowMs);
  void updateDisplayPower(uint32_t nowMs);
  void updateWifiPower(uint32_t nowMs);
  void wakeDisplay(uint32_t nowMs);
  const char* currentScreenName() const;

  SettingsStore settings_;
  BatteryService battery_;
  WifiService wifi_;
  TimeService timeService_;
  ScreenManager screenManager_;

  WatchScreen watchScreen_;
  MenuScreen mainMenuScreen_;
  MenuScreen settingsMenuScreen_;
  VolumeScreen volumeScreen_;
  WifiScreen wifiScreen_;
  BackgroundScreen backgroundScreen_;
  PowerScreen powerScreen_;
  DeviceInfoScreen deviceInfoScreen_;
  bool touchActive_ = false;
  uint32_t lastActivityMs_ = 0;
  uint32_t wifiDemandStartedMs_ = 0;
  DisplayPowerState displayPowerState_ = DisplayPowerState::Active;
};

}  // namespace iris
