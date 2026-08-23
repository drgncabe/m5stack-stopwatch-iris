#include "iris/screens/AxisCalibrationScreen.h"

#include <M5Unified.h>
#include <math.h>

#include "iris/Theme.h"
#include "iris/screens/ScreenManager.h"

namespace iris {

namespace {
constexpr uint32_t kSampleMs = 25;
constexpr uint32_t kSettlingMs = 800;
constexpr uint32_t kStableMs = 1250;
constexpr uint16_t kRequiredSamples = 60;
constexpr float kMaxGyroDps = 7.5f;
constexpr float kMaxAccelVariance = 0.012f;
constexpr float kMinGravity = 0.72f;
constexpr float kMaxGravity = 1.28f;
constexpr int kButtonY = 354;
constexpr int kButtonW = 138;
constexpr int kButtonH = 48;
constexpr AxisCalibrationScreen::Pose kPoses[] = {
    {"UP", "Point the top upward", "^", {0.0f, -1.0f, 0.0f}},
    {"DOWN", "Point the top downward", "v", {0.0f, 1.0f, 0.0f}},
    {"LEFT", "Point left side down", "<", {-1.0f, 0.0f, 0.0f}},
    {"RIGHT", "Point right side down", ">", {1.0f, 0.0f, 0.0f}},
};

float magnitude(const Vec3& vec) {
  return sqrtf((vec.x * vec.x) + (vec.y * vec.y) + (vec.z * vec.z));
}

Vec3 subtract(const Vec3& left, const Vec3& right) {
  return {left.x - right.x, left.y - right.y, left.z - right.z};
}
}  // namespace

AxisCalibrationScreen::AxisCalibrationScreen(SettingsStore& settings) : settings_(settings) {}

void AxisCalibrationScreen::enter() {
  mode_ = Mode::Summary;
  statusText_ = nullptr;
  statusUntilMs_ = 0;
  stableSinceMs_ = 0;
  stateStartedMs_ = 0;
  lastSampleMs_ = 0;
}

void AxisCalibrationScreen::update(uint32_t nowMs) {
  if (mode_ == Mode::Settling && nowMs - stateStartedMs_ >= kSettlingMs) {
    mode_ = Mode::WaitingForStability;
    stateStartedMs_ = nowMs;
    statusText_ = "Hold still";
    draw();
    return;
  }
  if (mode_ == Mode::WaitingForStability || mode_ == Mode::Sampling) {
    updateCapture(nowMs);
    return;
  }
  if (statusText_ && nowMs >= statusUntilMs_) {
    statusText_ = nullptr;
    draw();
  }
}

void AxisCalibrationScreen::draw() {
  const Theme theme = currentTheme(settings_);
  M5.Display.fillScreen(theme.background);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.drawString("IMU Calibration", M5.Display.width() / 2, 42);

  switch (mode_) {
    case Mode::Instructions:
    case Mode::Settling:
    case Mode::WaitingForStability:
    case Mode::Sampling:
      drawCapture(theme);
      break;
    case Mode::PoseComplete:
      drawPoseComplete(theme);
      break;
    case Mode::Complete:
      drawComplete(theme);
      break;
    case Mode::ClearConfirm:
      drawClearConfirm(theme);
      break;
    case Mode::Saved:
      drawSaved(theme);
      break;
    case Mode::Summary:
    default:
      drawSummary(theme);
      break;
  }
}

void AxisCalibrationScreen::handleTouch(int32_t x, int32_t y) {
  if (mode_ == Mode::Summary) {
    if (y >= 298 && y <= 342) {
      startCalibration();
    } else if (y >= 350 && y <= 394 && settings_.imuCalibrationValid()) {
      mode_ = Mode::ClearConfirm;
      draw();
    } else if (y >= 402 && y <= 446) {
      goBack();
    }
    return;
  }

  if (mode_ == Mode::Instructions) {
    beginPoseSettling();
    return;
  }

  if (mode_ == Mode::PoseComplete) {
    advanceAfterPose();
    return;
  }

  if (mode_ == Mode::Complete) {
    if (y >= kButtonY && y <= kButtonY + kButtonH) {
      if (x < 230) {
        startCalibration();
      } else {
        saveCalibration();
      }
    }
    return;
  }

  if (mode_ == Mode::ClearConfirm) {
    if (y >= kButtonY && y <= kButtonY + kButtonH) {
      if (x < 230) {
        mode_ = Mode::Summary;
        draw();
      } else {
        clearCalibration();
      }
    }
    return;
  }

  if (mode_ == Mode::Saved && y >= kButtonY && y <= kButtonY + kButtonH) {
    mode_ = Mode::Summary;
    draw();
  }
}

void AxisCalibrationScreen::onButtonA() {
  if (mode_ == Mode::PoseComplete) {
    retryCurrentPose();
    return;
  }
  if (mode_ == Mode::Instructions || mode_ == Mode::Settling ||
      mode_ == Mode::WaitingForStability || mode_ == Mode::Sampling ||
      mode_ == Mode::ClearConfirm || mode_ == Mode::Complete || mode_ == Mode::Saved) {
    mode_ = Mode::Summary;
    draw();
    return;
  }
  goBack();
}

void AxisCalibrationScreen::onButtonB() {
  switch (mode_) {
    case Mode::Summary:
      startCalibration();
      break;
    case Mode::Complete:
      saveCalibration();
      break;
    case Mode::ClearConfirm:
      clearCalibration();
      break;
    case Mode::Saved:
      mode_ = Mode::Summary;
      draw();
      break;
    case Mode::PoseComplete:
      mode_ = Mode::Summary;
      draw();
      break;
    case Mode::Instructions:
    case Mode::Settling:
    case Mode::WaitingForStability:
    case Mode::Sampling:
    default:
      break;
  }
}

void AxisCalibrationScreen::startCalibration() {
  pendingCalibration_ = settings_.imuCalibration();
  pendingCalibration_.valid = false;
  pendingCalibration_.version = ImuCalibrationData::kVersion;
  pendingCalibration_.sampleCount = 0;
  pendingCalibration_.gyroBias = {0.0f, 0.0f, 0.0f};
  poseIndex_ = 0;
  resetSamples();
  stableSinceMs_ = 0;
  statusText_ = nullptr;
  lastSampleMs_ = 0;
  capturedSamples_ = 0;
  beginPoseInstructions();
}

void AxisCalibrationScreen::beginPoseInstructions() {
  resetSamples();
  stableSinceMs_ = 0;
  lastSampleMs_ = 0;
  statusText_ = "Touch to start";
  stateStartedMs_ = millis();
  mode_ = Mode::Instructions;
  draw();
}

void AxisCalibrationScreen::beginPoseSettling() {
  resetSamples();
  stableSinceMs_ = 0;
  lastSampleMs_ = 0;
  statusText_ = "Settling...";
  stateStartedMs_ = millis();
  mode_ = Mode::Settling;
  draw();
}

void AxisCalibrationScreen::beginSampling(uint32_t nowMs) {
  resetSamples();
  stableSinceMs_ = nowMs;
  stateStartedMs_ = nowMs;
  statusText_ = "Sampling";
  mode_ = Mode::Sampling;
  draw();
}

void AxisCalibrationScreen::updateCapture(uint32_t nowMs) {
  if (lastSampleMs_ != 0 && nowMs - lastSampleMs_ < kSampleMs) return;
  lastSampleMs_ = nowMs;

  Vec3 accel;
  Vec3 gyro;
  imuReady_ = readImu(&accel, &gyro);
  if (!imuReady_) {
    statusText_ = "IMU unavailable";
    statusUntilMs_ = nowMs + 800;
    draw();
    return;
  }

  lastAccel_ = accel;
  lastGyro_ = gyro;
  if (!isStable(accel, gyro)) {
    resetSamples();
    stableSinceMs_ = 0;
    statusText_ = mode_ == Mode::Sampling ? "Movement detected" : "Hold still";
    mode_ = Mode::WaitingForStability;
    draw();
    return;
  }

  if (stableSinceMs_ == 0) stableSinceMs_ = nowMs;
  if (mode_ == Mode::WaitingForStability) {
    statusText_ = nowMs - stableSinceMs_ >= kStableMs ? "Stable" : "Stabilizing...";
    draw();
    if (nowMs - stableSinceMs_ >= kStableMs) {
      beginSampling(nowMs);
    }
    return;
  }

  addSample(accel, gyro);
  statusText_ = "Sampling";
  draw();

  if (samples_.count >= kRequiredSamples && accelVariance() <= kMaxAccelVariance) {
    completePose();
  }
}

void AxisCalibrationScreen::completePose() {
  const Vec3 reference = meanAccel();
  const Vec3 gyro = meanGyro();
  capturedAccel_ = reference;
  capturedGyro_ = gyro;
  capturedSamples_ = samples_.count;
  pulse(88, 18);

  resetSamples();
  stableSinceMs_ = 0;
  statusText_ = "Captured";
  stateStartedMs_ = millis();
  mode_ = Mode::PoseComplete;
  draw();
}

void AxisCalibrationScreen::retryCurrentPose() {
  beginPoseInstructions();
}

void AxisCalibrationScreen::advanceAfterPose() {
  assignPoseReference(poseIndex_, capturedAccel_);
  pendingCalibration_.gyroBias.x += capturedGyro_.x;
  pendingCalibration_.gyroBias.y += capturedGyro_.y;
  pendingCalibration_.gyroBias.z += capturedGyro_.z;
  pendingCalibration_.sampleCount += capturedSamples_;
  poseIndex_++;
  if (poseIndex_ >= sizeof(kPoses) / sizeof(kPoses[0])) {
    const float poseCount = static_cast<float>(sizeof(kPoses) / sizeof(kPoses[0]));
    pendingCalibration_.gyroBias.x /= poseCount;
    pendingCalibration_.gyroBias.y /= poseCount;
    pendingCalibration_.gyroBias.z /= poseCount;
    pendingCalibration_.accelOffset =
        subtract(pendingCalibration_.upReference, kPoses[0].expected);
    pendingCalibration_.calibratedAtMs = millis();
    mode_ = Mode::Complete;
    draw();
    return;
  }
  beginPoseInstructions();
}

void AxisCalibrationScreen::saveCalibration() {
  settings_.saveImuCalibration(pendingCalibration_);
  pulse(96, 24);
  mode_ = Mode::Saved;
  draw();
}

void AxisCalibrationScreen::clearCalibration() {
  settings_.clearImuCalibration();
  pulse(70, 18);
  mode_ = Mode::Summary;
  statusText_ = "Calibration cleared";
  statusUntilMs_ = millis() + 1200;
  draw();
}

void AxisCalibrationScreen::goBack() {
  if (manager_) manager_->show(ScreenId::Developer);
}

void AxisCalibrationScreen::drawSummary(const Theme& theme) {
  const auto& data = settings_.imuCalibration();
  int y = 86;
  M5.Display.setTextDatum(top_left);
  drawMetric(62, y, "Status", data.valid ? "Calibrated" : "Not calibrated", theme);
  drawMetric(62, y, "Last", data.valid ? formatCalibrationAge(data.calibratedAtMs) : "Never",
             theme);
  drawMetric(62, y, "Samples", String(data.sampleCount), theme);
  drawMetric(62, y, "Accel", formatVec(data.accelOffset), theme);
  drawMetric(62, y, "Gyro", formatVec(data.gyroBias), theme);

  if (statusText_) {
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextColor(theme.accent, theme.background);
    M5.Display.drawString(statusText_, M5.Display.width() / 2, 268);
  }

  drawButton(116, 298, 234, 44, data.valid ? "Recalibrate" : "Calibrate", theme.selected,
             theme.foreground);
  drawButton(116, 350, 234, 44, "Clear", data.valid ? theme.button : theme.panel,
             data.valid ? theme.foreground : theme.muted);
  drawButton(116, 402, 234, 44, "Back", theme.button, theme.foreground);
}

void AxisCalibrationScreen::drawCapture(const Theme& theme) {
  const Pose& pose = kPoses[poseIndex_];
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(theme.muted, theme.background);
  M5.Display.drawString(String("Step ") + String(poseIndex_ + 1) + "/" +
                            String(sizeof(kPoses) / sizeof(kPoses[0])),
                        M5.Display.width() / 2, 84);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.drawString(pose.instruction, M5.Display.width() / 2, 118);

  M5.Display.setFont(&fonts::FreeSansBold24pt7b);
  M5.Display.setTextColor(theme.accent, theme.background);
  M5.Display.drawString(pose.icon, M5.Display.width() / 2, 188);

  M5.Display.setFont(&fonts::FreeSansBold12pt7b);
  M5.Display.setTextColor(theme.foreground, theme.background);
  const char* stateText = statusText_ ? statusText_ : "Waiting for stability";
  M5.Display.drawString(stateText, M5.Display.width() / 2, 252);

  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(theme.muted, theme.background);
  if (mode_ == Mode::Instructions) {
    M5.Display.drawString("Position the device, then touch.", M5.Display.width() / 2, 294);
    M5.Display.drawString("Sampling starts after a short pause.", M5.Display.width() / 2, 326);
  } else if (mode_ == Mode::Settling) {
    M5.Display.drawString("Release the screen.", M5.Display.width() / 2, 294);
    M5.Display.drawString("Then hold Iris still.", M5.Display.width() / 2, 326);
  } else if (mode_ == Mode::WaitingForStability) {
    const uint32_t stableMs = stableSinceMs_ == 0 ? 0 : millis() - stableSinceMs_;
    M5.Display.drawString(formatVec(lastAccel_), M5.Display.width() / 2, 294);
    M5.Display.drawString(String("Stable ") + String(min(stableMs, kStableMs)) + "/" +
                              String(kStableMs) + " ms",
                          M5.Display.width() / 2, 326);
  } else {
    M5.Display.drawString(formatVec(lastAccel_), M5.Display.width() / 2, 294);
    M5.Display.drawString(String(samples_.count) + "/" + String(kRequiredSamples) + " samples",
                          M5.Display.width() / 2, 326);
  }
  M5.Display.drawString("A: Cancel", M5.Display.width() / 2, 414);
}

void AxisCalibrationScreen::drawPoseComplete(const Theme& theme) {
  const Pose& pose = kPoses[poseIndex_];
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSansBold12pt7b);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.drawString(String(pose.name) + " complete", M5.Display.width() / 2, 94);

  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(theme.muted, theme.background);
  M5.Display.drawString(String(capturedSamples_) + " samples captured",
                        M5.Display.width() / 2, 132);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.drawString("Accel", M5.Display.width() / 2, 174);
  M5.Display.setTextColor(theme.muted, theme.background);
  M5.Display.drawString(formatVec(capturedAccel_), M5.Display.width() / 2, 202);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.drawString("Gyro", M5.Display.width() / 2, 244);
  M5.Display.setTextColor(theme.muted, theme.background);
  M5.Display.drawString(formatVec(capturedGyro_), M5.Display.width() / 2, 272);
  M5.Display.drawString("Touch: Continue", M5.Display.width() / 2, 326);
  M5.Display.drawString("A: Retry   B: Cancel", M5.Display.width() / 2, 414);
}

void AxisCalibrationScreen::drawComplete(const Theme& theme) {
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSansBold12pt7b);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.drawString("Calibration complete", M5.Display.width() / 2, 104);

  M5.Display.setFont(&fonts::FreeSans9pt7b);
  int y = 146;
  for (size_t i = 0; i < sizeof(kPoses) / sizeof(kPoses[0]); ++i) {
    M5.Display.drawString(String(kPoses[i].name) + " captured", M5.Display.width() / 2, y);
    y += 30;
  }
  M5.Display.setTextColor(theme.muted, theme.background);
  M5.Display.drawString("B: Save", M5.Display.width() / 2, 316);
  drawButton(76, kButtonY, kButtonW, kButtonH, "Retry", theme.button, theme.foreground);
  drawButton(252, kButtonY, kButtonW, kButtonH, "Save", theme.selected, theme.foreground);
}

void AxisCalibrationScreen::drawClearConfirm(const Theme& theme) {
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSansBold12pt7b);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.drawString("Clear calibration?", M5.Display.width() / 2, 148);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(theme.muted, theme.background);
  M5.Display.drawString("Existing sensor offsets will reset.", M5.Display.width() / 2, 204);
  M5.Display.drawString("A: Cancel   B: Clear", M5.Display.width() / 2, 276);
  drawButton(76, kButtonY, kButtonW, kButtonH, "Cancel", theme.button, theme.foreground);
  drawButton(252, kButtonY, kButtonW, kButtonH, "Clear", theme.selected, theme.foreground);
}

void AxisCalibrationScreen::drawSaved(const Theme& theme) {
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSansBold12pt7b);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.drawString("Calibration saved", M5.Display.width() / 2, 156);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(theme.muted, theme.background);
  M5.Display.drawString("Motion apps now use these offsets.", M5.Display.width() / 2, 214);
  drawButton(164, kButtonY, kButtonW, kButtonH, "Done", theme.selected, theme.foreground);
}

void AxisCalibrationScreen::drawMetric(int x, int& y, const char* label, const String& value,
                                       const Theme& theme) {
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(theme.muted, theme.background);
  M5.Display.drawString(label, x, y);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.drawString(value, x + 116, y);
  y += 28;
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

bool AxisCalibrationScreen::readImu(Vec3* accel, Vec3* gyro) {
  if (!M5.Imu.isEnabled()) return false;
  M5.Imu.update();
  if (!M5.Imu.getAccel(&accel->x, &accel->y, &accel->z)) return false;
  if (!M5.Imu.getGyro(&gyro->x, &gyro->y, &gyro->z)) {
    *gyro = {0.0f, 0.0f, 0.0f};
  }
  return true;
}

bool AxisCalibrationScreen::isStable(const Vec3& accel, const Vec3& gyro) const {
  const float gravity = magnitude(accel);
  if (gravity < kMinGravity || gravity > kMaxGravity) return false;
  return magnitude(gyro) <= kMaxGyroDps;
}

Vec3 AxisCalibrationScreen::meanAccel() const {
  if (samples_.count == 0) return {0.0f, 0.0f, 0.0f};
  const float count = static_cast<float>(samples_.count);
  return {samples_.accelSum.x / count, samples_.accelSum.y / count,
          samples_.accelSum.z / count};
}

Vec3 AxisCalibrationScreen::meanGyro() const {
  if (samples_.count == 0) return {0.0f, 0.0f, 0.0f};
  const float count = static_cast<float>(samples_.count);
  return {samples_.gyroSum.x / count, samples_.gyroSum.y / count,
          samples_.gyroSum.z / count};
}

float AxisCalibrationScreen::accelVariance() const {
  if (samples_.count == 0) return 0.0f;
  const float count = static_cast<float>(samples_.count);
  const Vec3 mean = meanAccel();
  const float vx = (samples_.accelSquareSum.x / count) - (mean.x * mean.x);
  const float vy = (samples_.accelSquareSum.y / count) - (mean.y * mean.y);
  const float vz = (samples_.accelSquareSum.z / count) - (mean.z * mean.z);
  return max(0.0f, vx + vy + vz);
}

void AxisCalibrationScreen::resetSamples() {
  samples_ = {};
}

void AxisCalibrationScreen::addSample(const Vec3& accel, const Vec3& gyro) {
  samples_.accelSum.x += accel.x;
  samples_.accelSum.y += accel.y;
  samples_.accelSum.z += accel.z;
  samples_.accelSquareSum.x += accel.x * accel.x;
  samples_.accelSquareSum.y += accel.y * accel.y;
  samples_.accelSquareSum.z += accel.z * accel.z;
  samples_.gyroSum.x += gyro.x;
  samples_.gyroSum.y += gyro.y;
  samples_.gyroSum.z += gyro.z;
  samples_.count++;
}

void AxisCalibrationScreen::assignPoseReference(size_t pose, const Vec3& reference) {
  switch (pose) {
    case 0:
      pendingCalibration_.upReference = reference;
      break;
    case 1:
      pendingCalibration_.downReference = reference;
      break;
    case 2:
      pendingCalibration_.leftReference = reference;
      break;
    case 3:
      pendingCalibration_.rightReference = reference;
      break;
    default:
      break;
  }
}

String AxisCalibrationScreen::formatVec(const Vec3& vec) const {
  return String(vec.x, 2) + ", " + String(vec.y, 2) + ", " + String(vec.z, 2);
}

String AxisCalibrationScreen::formatCalibrationAge(uint32_t calibratedAtMs) const {
  if (calibratedAtMs == 0) return "Unknown";
  const uint32_t ageSeconds = (millis() - calibratedAtMs) / 1000UL;
  if (ageSeconds < 60) return String(ageSeconds) + "s ago";
  if (ageSeconds < 3600) return String(ageSeconds / 60) + "m ago";
  return String(ageSeconds / 3600) + "h ago";
}

void AxisCalibrationScreen::pulse(uint8_t strength, uint32_t durationMs) {
  M5.Power.setVibration(strength);
  delay(durationMs);
  M5.Power.setVibration(0);
}

}  // namespace iris
