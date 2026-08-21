#pragma once

#include "iris/screens/Screen.h"
#include "iris/services/SettingsStore.h"

namespace iris {

class BootloaderScreen : public Screen {
 public:
  explicit BootloaderScreen(SettingsStore& settings) : settings_(settings) {}

  void enter() override;
  void update(uint32_t nowMs) override;
  void draw() override;
  void handleTouch(int32_t x, int32_t y) override;
  void onButtonA() override;
  void onButtonB() override;

 private:
  void goBack();
  void rebootToBootloader();
  void drawButton(int x, int y, int w, int h, const char* label, uint16_t fill, uint16_t text);

  SettingsStore& settings_;
  bool rebooting_ = false;
};

}  // namespace iris
