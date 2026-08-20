#pragma once

#include "iris/screens/Screen.h"
#include "iris/services/BatteryService.h"
#include "iris/services/TimeService.h"

namespace iris {

class WatchScreen : public Screen {
 public:
  WatchScreen(TimeService& timeService, BatteryService& battery)
      : timeService_(timeService), battery_(battery) {}

  void enter() override;
  void update(uint32_t nowMs) override;
  void draw() override;
  void handleTouch(int32_t x, int32_t y) override;
  void onButtonA() override;
  void onButtonB() override;

 private:
  void drawStaticLayout();
  void drawBattery();

  TimeService& timeService_;
  BatteryService& battery_;
  uint32_t lastDrawMs_ = 0;
  bool layoutDrawn_ = false;
};

}  // namespace iris
