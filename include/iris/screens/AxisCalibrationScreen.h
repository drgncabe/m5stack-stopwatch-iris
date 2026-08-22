#pragma once

#include <Arduino.h>

#include "iris/screens/Screen.h"
#include "iris/services/SettingsStore.h"

namespace iris {

class AxisCalibrationScreen : public Screen {
 public:
  explicit AxisCalibrationScreen(SettingsStore& settings);

  void enter() override;
  void update(uint32_t nowMs) override;
  void draw() override;
  void handleTouch(int32_t x, int32_t y) override;
  void onButtonA() override;
  void onButtonB() override;

 private:
  void readSample();
  void captureCalibration();
  void resetCalibration();
  void goBack();
  void drawButton(int x, int y, int w, int h, const char* label, uint16_t fill, uint16_t text);

  SettingsStore& settings_;
  float ax_ = 0.0f;
  float ay_ = 0.0f;
  float az_ = 0.0f;
  float pitch_ = 0.0f;
  float roll_ = 0.0f;
  bool imuReady_ = false;
  uint32_t lastSampleMs_ = 0;
};

}  // namespace iris
