#pragma once

#include "iris/apps/connectivity/MediaRemoteApp.h"
#include "iris/apps/media/BadgeApp.h"
#include "iris/apps/development/DevelopmentApp.h"
#include "iris/apps/settings/SettingsApp.h"
#include "iris/apps/tools/StopwatchApp.h"
#include "iris/apps/watch/WatchApp.h"
#include "iris/core/AppManager.h"
#include "iris/core/EventBus.h"
#include "iris/core/ServiceManager.h"
#include "iris/screens/AxisCalibrationScreen.h"
#include "iris/screens/BadgeScreen.h"
#include "iris/screens/BootloaderScreen.h"
#include "iris/screens/BackgroundScreen.h"
#include "iris/screens/DateTimeScreen.h"
#include "iris/screens/DeviceInfoScreen.h"
#include "iris/screens/FidgetScreens.h"
#include "iris/screens/HardwareDiagnosticsScreen.h"
#include "iris/screens/MediaRemoteScreen.h"
#include "iris/screens/MenuScreen.h"
#include "iris/screens/PowerScreen.h"
#include "iris/screens/RagnarLinkScreen.h"
#include "iris/screens/ScreenManager.h"
#include "iris/screens/StopwatchScreens.h"
#include "iris/screens/VolumeScreen.h"
#include "iris/screens/WatchScreen.h"
#include "iris/screens/WifiScannerScreen.h"
#include "iris/screens/WifiScreen.h"
#include "iris/services/BatteryService.h"
#include "iris/services/BadgeService.h"
#include "iris/services/BluetoothService.h"
#include "iris/services/OrientationService.h"
#include "iris/services/NetworkScanService.h"
#include "iris/services/PowerManager.h"
#include "iris/services/RagnarLinkService.h"
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
  void updateSystemEvents(bool previousWifiConnected, uint8_t previousRotation);
  void handleControlCommand(const String& command);
  String buildControlSnapshot() const;
  void adjustVolume(int delta);
  void adjustBrightness(int delta);
  void adjustDimTimeout(int delta);
  void adjustSleepTimeout(int delta);
  void adjustTouchDelay(int delta);
  void setIndicatorLight(bool enabled);
  void enterBootloaderFromWeb();
  void nextTheme();
  void nextComplication();
  void nextBadgeMode();
  void resetTouch();
  void showWatchIfActive();
  void noteActivity(uint32_t nowMs);
  void updateDisplayPower(uint32_t nowMs);
  void updateWifiPower(uint32_t nowMs);
  void wakeDisplay(uint32_t nowMs);
  bool shouldUpdateForeground(uint32_t nowMs, const AppDescriptor* app);
  const char* currentScreenName() const;

  SettingsStore settings_;
  BadgeService badge_;
  BluetoothService bluetooth_;
  BatteryService battery_;
  OrientationService orientation_;
  StatusLightService statusLight_;
  WifiService wifi_;
  NetworkScanService networkScanner_;
  RagnarLinkService ragnar_;
  TimeService timeService_;
  PowerManager power_;
  EventBus events_;
  ServiceManager services_;
  ScreenManager screenManager_;
  AppManager appManager_;

  DevelopmentApp developmentApp_;
  SettingsApp settingsApp_;
  WatchScreen watchScreen_;
  WatchApp watchApp_;
  StopwatchEngine stopwatchEngine_;
  StopwatchScreen stopwatchScreen_;
  StopwatchLapHistoryScreen stopwatchLapHistoryScreen_;
  StopwatchApp stopwatchApp_;
  WifiScannerScreen wifiScannerScreen_;
  RagnarLinkScreen ragnarLinkScreen_;
  BadgeScreen badgeScreen_;
  BadgeApp badgeApp_;
  MediaRemoteScreen mediaRemoteScreen_;
  MediaRemoteApp mediaRemoteApp_;
  MenuScreen mainMenuScreen_;
  MenuScreen settingsMenuScreen_;
  VolumeScreen volumeScreen_;
  WifiScreen wifiScreen_;
  DateTimeScreen dateTimeScreen_;
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
  uint32_t lastForegroundUpdateMs_ = 0;
  uint32_t wifiDemandStartedMs_ = 0;
  bool batteryLowPublished_ = false;
};

}  // namespace iris
