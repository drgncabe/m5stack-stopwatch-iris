#pragma once

#include "iris/screens/Screen.h"
#include "iris/services/TimeService.h"

namespace iris {

class WatchScreen : public Screen {
 public:
  explicit WatchScreen(TimeService& timeService) : timeService_(timeService) {}

  void enter() override;
  void update(uint32_t nowMs) override;
  void draw() override;
  void handleTouch(int32_t x, int32_t y) override;
  void onButtonA() override;
  void onButtonB() override;

 private:
  TimeService& timeService_;
  uint32_t lastDrawMs_ = 0;
};

}  // namespace iris
