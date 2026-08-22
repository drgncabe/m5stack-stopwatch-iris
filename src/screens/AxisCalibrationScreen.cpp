#include "iris/screens/AxisCalibrationScreen.h"

#include <M5Unified.h>
#include <math.h>

#include "iris/Theme.h"
#include "iris/screens/ScreenManager.h"

namespace iris {

namespace {
constexpr uint32_t kSampleMs = 180;
constexpr int kResetX = 58;
constexpr int kSaveX = 248;
constexpr int kButtonY = 348;
constexpr int kButtonWidth = 160;
constexpr int kButtonHeight = 54;
constexpr float kRadToDeg = 57.2957795f;
}  // namespace

AxisCalibrationScreen::AxisCalibrationScreen(SettingsStore& settings) : settings_(settings) {}

void AxisCalibrationScreen::enter() {
  lastSampleMs_ = 0;
  readSample();
}

void AxisCalibrationScreen::update(uint32_t nowMs) {
  if (lastSampleMs_ != 0 && nowMs - lastSampleMs_ < kSampleMs) return;
  readSample();
  draw();
}

void AxisCalibrationScreen::draw() {
  const Theme theme = currentTheme(settings_);
  M5.Display.fillScreen(theme.background);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.drawString("Axis Calibration", M5.Display.width() / 2, 48);

  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextDatum(top_left);
  const int labelX = 68;
  const int valueX = 214;
  int y = 104;

  auto drawMetric = [&](const char* label, const String& value) {
    M5.Display.setTextColor(theme.muted, theme.background);
    M5.Display.drawString(label, labelX, y);
    M5.Display.setTextColor(theme.foreground, theme.background);
    M5.Display.drawString(value, valueX, y);
    y += 30;
  };

  drawMetric("IMU", imuReady_ ? "Ready" : "Unavailable");
  drawMetric("Pitch", String(pitch_, 1) + " deg");
  drawMetric("Roll", String(roll_, 1) + " deg");
  drawMetric("Yaw", "Unavailable");
  drawMetric("Accel X", String(ax_, 3) + " g");
  drawMetric("Accel Y", String(ay_, 3) + " g");
  drawMetric("Accel Z", String(az_, 3) + " g");
  drawMetric("Offset X", String(settings_.accelOffsetX(), 3));
  drawMetric("Offset Y", String(settings_.accelOffsetY(), 3));
  drawMetric("Offset Z", String(settings_.accelOffsetZ(), 3));

  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(theme.muted, theme.background);
  M5.Display.drawString("A: Back     B: Capture", M5.Display.width() / 2, 424);
  drawButton(kResetX, kButtonY, kButtonWidth, kButtonHeight, "Reset", theme.button, theme.foreground);
  drawButton(kSaveX, kButtonY, kButtonWidth, kButtonHeight, "Capture", theme.selected, theme.foreground);
}

void AxisCalibrationScreen::handleTouch(int32_t x, int32_t y) {
  if (y < kButtonY || y > kButtonY + kButtonHeight) return;
  if (x >= kResetX && x <= kResetX + kButtonWidth) {
    resetCalibration();
  } else if (x >= kSaveX && x <= kSaveX + kButtonWidth) {
    captureCalibration();
  }
}

void AxisCalibrationScreen::onButtonA() {
  goBack();
}

void AxisCalibrationScreen::onButtonB() {
  captureCalibration();
}

void AxisCalibrationScreen::readSample() {
  lastSampleMs_ = millis();
  imuReady_ = M5.Imu.isEnabled();
  if (!imuReady_) return;

  M5.Imu.update();
  float rawX = 0.0f;
  float rawY = 0.0f;
  float rawZ = 0.0f;
  if (!M5.Imu.getAccel(&rawX, &rawY, &rawZ)) {
    imuReady_ = false;
    return;
  }

  ax_ = rawX + settings_.accelOffsetX();
  ay_ = rawY + settings_.accelOffsetY();
  az_ = rawZ + settings_.accelOffsetZ();
  pitch_ = atan2f(ax_, sqrtf((ay_ * ay_) + (az_ * az_))) * kRadToDeg;
  roll_ = atan2f(ay_, az_) * kRadToDeg;
}

void AxisCalibrationScreen::captureCalibration() {
  if (!imuReady_) return;

  M5.Imu.update();
  float rawX = 0.0f;
  float rawY = 0.0f;
  float rawZ = 0.0f;
  if (!M5.Imu.getAccel(&rawX, &rawY, &rawZ)) return;

  settings_.setAccelCalibration(-rawX, -rawY, 1.0f - rawZ);
  M5.Power.setVibration(90);
  delay(16);
  M5.Power.setVibration(0);
  readSample();
  draw();
}

void AxisCalibrationScreen::resetCalibration() {
  settings_.resetAccelCalibration();
  M5.Power.setVibration(70);
  delay(12);
  M5.Power.setVibration(0);
  readSample();
  draw();
}

void AxisCalibrationScreen::goBack() {
  if (manager_) manager_->show(ScreenId::Developer);
}

void AxisCalibrationScreen::drawButton(int x, int y, int w, int h, const char* label,
                                       uint16_t fill, uint16_t text) {
  M5.Display.fillRoundRect(x, y, w, h, 14, fill);
  M5.Display.drawRoundRect(x, y, w, h, 14, text);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(text, fill);
  M5.Display.drawString(label, x + (w / 2), y + (h / 2));
}

}  // namespace iris
