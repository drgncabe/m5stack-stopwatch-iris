#pragma once

#include "iris/screens/Screen.h"
#include "iris/services/SettingsStore.h"

namespace iris {

class PowerScreen : public Screen {
 public:
  explicit PowerScreen(SettingsStore& settings) : settings_(settings) {}

  void enter() override;
  void update(uint32_t nowMs) override;
  void draw() override;
  void previewTouch(int32_t x, int32_t y) override;
  void handleTouch(int32_t x, int32_t y) override;
  void onButtonA() override;
  void onButtonB() override;

 private:
  void activateSelected();
  void selectRow(size_t index);
  void drawRow(size_t index, bool selected);
  int rowAt(int32_t x, int32_t y) const;
  void cycleBrightness();
  void cycleDimTimeout();
  void cycleSleepTimeout();
  void cycleTouchDelay();
  const char* brightnessName() const;
  const char* dimTimeoutName() const;
  const char* sleepTimeoutName() const;
  const char* touchDelayName() const;
  void goBack();

  SettingsStore& settings_;
  size_t selected_ = 0;
};

}  // namespace iris
