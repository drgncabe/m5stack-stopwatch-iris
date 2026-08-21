#pragma once

#include "iris/screens/Screen.h"
#include "iris/services/BatteryService.h"
#include "iris/services/SettingsStore.h"
#include "iris/services/TimeService.h"
#include "iris/services/WifiService.h"

namespace iris {

class WatchScreen : public Screen {
 public:
  WatchScreen(TimeService& timeService, BatteryService& battery, WifiService& wifi,
              SettingsStore& settings)
      : timeService_(timeService), battery_(battery), wifi_(wifi), settings_(settings) {}

  void enter() override;
  void update(uint32_t nowMs) override;
  void draw() override;
  void handleTouch(int32_t x, int32_t y) override;
  void onButtonA() override;
  void onButtonB() override;

 private:
  void drawStaticLayout();
  void drawBattery();
  void drawWifi();
  void drawDate(const DateTimeSnapshot& dt);
  void drawTime(const DateTimeSnapshot& dt);
  void drawUnsetTime();

  TimeService& timeService_;
  BatteryService& battery_;
  WifiService& wifi_;
  SettingsStore& settings_;
  uint32_t lastDrawMs_ = 0;
  bool layoutDrawn_ = false;
  bool previousValid_ = false;
  int previousMinute_ = -1;
  int previousSecond_ = -1;
  int previousDay_ = -1;
  String previousBattery_;
  String previousWifi_;
};

}  // namespace iris
