#pragma once

#include "iris/core/AppManager.h"
#include "iris/screens/ScreenManager.h"

namespace iris {

class SettingsApp : public IrisApplication {
 public:
  explicit SettingsApp(ScreenManager& screens) : screens_(screens) {}

  const char* id() const override { return "system.settings"; }
  const char* name() const override { return "Settings"; }
  bool onButtonA() override;
  bool onButtonB() override;
  bool onTouch(int32_t x, int32_t y) override;

 private:
  ScreenManager& screens_;
};

}  // namespace iris
