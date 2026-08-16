#pragma once

#include "iris/screens/Screen.h"
#include "iris/services/TimeService.h"
#include "iris/services/WifiService.h"

namespace iris {

class DeviceInfoScreen : public Screen {
 public:
  DeviceInfoScreen(WifiService& wifi, TimeService& timeService)
      : wifi_(wifi), timeService_(timeService) {}

  void enter() override;
  void update(uint32_t nowMs) override;
  void draw() override;
  void handleTouch(int32_t x, int32_t y) override;
  void onButtonA() override;
  void onButtonB() override;

 private:
  void goBack();

  WifiService& wifi_;
  TimeService& timeService_;
};

}  // namespace iris
