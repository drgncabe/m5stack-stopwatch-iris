#pragma once

#include "iris/screens/Screen.h"
#include "iris/services/SettingsStore.h"

namespace iris {

class VolumeScreen : public Screen {
 public:
  explicit VolumeScreen(SettingsStore& settings) : settings_(settings) {}

  void enter() override;
  void update(uint32_t nowMs) override;
  void draw() override;
  void handleTouch(int32_t x, int32_t y) override;
  void onButtonA() override;
  void onButtonB() override;

 private:
  void changeVolume(int delta);
  void goBack();

  SettingsStore& settings_;
};

}  // namespace iris
