#pragma once

#include "iris/core/AppManager.h"
#include "iris/screens/WatchScreen.h"

namespace iris {

class WatchApp : public IrisApplication {
 public:
  explicit WatchApp(WatchScreen& screen) : screen_(screen) {}

  const char* id() const override { return "system.watch"; }
  const char* name() const override { return "Watch"; }
  bool onButtonA() override;
  bool onButtonB() override;
  bool onTouch(int32_t x, int32_t y) override;

 private:
  WatchScreen& screen_;
};

}  // namespace iris
