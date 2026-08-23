#pragma once

#include "iris/core/AppManager.h"
#include "iris/core/ServiceRegistry.h"
#include "iris/screens/AxisCalibrationScreen.h"
#include "iris/screens/BootloaderScreen.h"
#include "iris/screens/BackgroundScreen.h"
#include "iris/screens/DeviceInfoScreen.h"
#include "iris/screens/FidgetScreens.h"
#include "iris/screens/HardwareDiagnosticsScreen.h"
#include "iris/screens/MenuScreen.h"
#include "iris/screens/PowerScreen.h"
#include "iris/screens/ScreenManager.h"
#include "iris/screens/VolumeScreen.h"
#include "iris/screens/WatchScreen.h"
#include "iris/screens/WifiScreen.h"
#include "iris/services/BatteryService.h"
#include "iris/services/OrientationService.h"
#include "iris/services/PowerManager.h"
#include "iris/services/SettingsStore.h"
#include "iris/services/StatusLightService.h"
#include "iris/services/TimeService.h"
#include "iris/services/WifiService.h"

namespace iris {

class App {
 public:
  App();

  void begin();
  void update();

 private:
  static void handleControlCommand(void* context, const String& command);
  static String buildControlSnapshot(void* context);

  void registerServices();
  void registerScreens();
  void registerApps();
  void handleControlCommand(const String& command);
  String buildControlSnapshot() const;
  void adjustVolume(int delta);
  void adjustBrightness(int delta);
  void adjustDimTimeout(int delta);
  void adjustSleepTimeout(int delta);
  void adjustTouchDelay(int delta);
  void setIndicatorLight(bool enabled);
  void nextTheme();
  void nextComplication();
  void resetTouch();
  void showWatchIfActive();
  void noteActivity(uint32_t nowMs);
  void updateDisplayPower(uint32_t nowMs);
  void updateWifiPower(uint32_t nowMs);
  void wakeDisplay(uint32_t nowMs);
  const char* currentScreenName() const;

  SettingsStore settings_;
  BatteryService battery_;
  OrientationService orientation_;
  StatusLightService statusLight_;
  WifiService wifi_;
  TimeService timeService_;
  PowerManager power_;
  ServiceRegistry services_;
  ScreenManager screenManager_;
  AppManager appManager_;

  WatchScreen watchScreen_;
  MenuScreen mainMenuScreen_;
  MenuScreen settingsMenuScreen_;
  VolumeScreen volumeScreen_;
  WifiScreen wifiScreen_;
  BackgroundScreen backgroundScreen_;
  PowerScreen powerScreen_;
  MenuScreen fidgetsMenuScreen_;
  WheelFidgetScreen wheelFidgetScreen_;
  PoppersFidgetScreen poppersFidgetScreen_;
  SpinnerFidgetScreen spinnerFidgetScreen_;
  GravityBallFidgetScreen gravityBallFidgetScreen_;
  MenuScreen developerMenuScreen_;
  AxisCalibrationScreen axisCalibrationScreen_;
  BootloaderScreen bootloaderScreen_;
  HardwareDiagnosticsScreen hardwareDiagnosticsScreen_;
  DeviceInfoScreen deviceInfoScreen_;
  bool touchActive_ = false;
  bool touchPreviewed_ = false;
  bool touchHandled_ = false;
  bool touchMoved_ = false;
  int32_t touchStartX_ = 0;
  int32_t touchStartY_ = 0;
  uint32_t touchStartMs_ = 0;
  uint32_t wifiDemandStartedMs_ = 0;
};

}  // namespace iris
