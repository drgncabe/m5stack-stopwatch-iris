#pragma once

#include "iris/core/AppManager.h"
#include "iris/screens/StopwatchScreens.h"

namespace iris {

class StopwatchApp : public IrisApplication {
 public:
  explicit StopwatchApp(StopwatchScreen& screen) : screen_(screen) {}

  const char* id() const override { return "tools.stopwatch"; }
  const char* name() const override { return "Stopwatch"; }
  void onPause() override { screen_.cancelFeedback(); }
  void onStop() override { screen_.cancelFeedback(); }
  bool onButtonA() override { return false; }
  bool onButtonB() override { return false; }
  bool onTouch(int32_t, int32_t) override { return false; }

 private:
  StopwatchScreen& screen_;
};

}  // namespace iris
