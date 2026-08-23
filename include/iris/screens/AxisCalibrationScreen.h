#pragma once

#include <Arduino.h>

#include "iris/screens/Screen.h"
#include "iris/services/SettingsStore.h"

namespace iris {

struct Theme;

class AxisCalibrationScreen : public Screen {
 public:
  explicit AxisCalibrationScreen(SettingsStore& settings);

  void enter() override;
  void update(uint32_t nowMs) override;
  void draw() override;
  void handleTouch(int32_t x, int32_t y) override;
  void onButtonA() override;
  void onButtonB() override;

  struct Pose {
    const char* name;
    const char* instruction;
    const char* icon;
    Vec3 expected;
  };

 private:
  enum class Mode : uint8_t {
    Summary,
    Instructions,
    Settling,
    WaitingForStability,
    Sampling,
    PoseComplete,
    Complete,
    ClearConfirm,
    Saved,
  };

  struct SampleAccumulator {
    Vec3 accelSum{0.0f, 0.0f, 0.0f};
    Vec3 accelSquareSum{0.0f, 0.0f, 0.0f};
    Vec3 gyroSum{0.0f, 0.0f, 0.0f};
    uint16_t count = 0;
  };

  void startCalibration();
  void beginPoseInstructions();
  void beginPoseSettling();
  void beginSampling(uint32_t nowMs);
  void updateCapture(uint32_t nowMs);
  void completePose();
  void retryCurrentPose();
  void advanceAfterPose();
  void saveCalibration();
  void clearCalibration();
  void goBack();
  void drawSummary(const Theme& theme);
  void drawCapture(const Theme& theme);
  void drawPoseComplete(const Theme& theme);
  void drawComplete(const Theme& theme);
  void drawClearConfirm(const Theme& theme);
  void drawSaved(const Theme& theme);
  void drawMetric(int x, int& y, const char* label, const String& value,
                  const Theme& theme);
  void drawButton(int x, int y, int w, int h, const char* label, uint16_t fill, uint16_t text);
  bool readImu(Vec3* accel, Vec3* gyro);
  bool isStable(const Vec3& accel, const Vec3& gyro) const;
  Vec3 meanAccel() const;
  Vec3 meanGyro() const;
  float accelVariance() const;
  void resetSamples();
  void addSample(const Vec3& accel, const Vec3& gyro);
  void assignPoseReference(size_t pose, const Vec3& reference);
  String formatVec(const Vec3& vec) const;
  String formatCalibrationAge(uint32_t calibratedAtMs) const;
  void pulse(uint8_t strength, uint32_t durationMs);

  SettingsStore& settings_;
  Mode mode_ = Mode::Summary;
  ImuCalibrationData pendingCalibration_;
  SampleAccumulator samples_;
  Vec3 lastAccel_{0.0f, 0.0f, 0.0f};
  Vec3 lastGyro_{0.0f, 0.0f, 0.0f};
  Vec3 capturedAccel_{0.0f, 0.0f, 0.0f};
  Vec3 capturedGyro_{0.0f, 0.0f, 0.0f};
  bool imuReady_ = false;
  size_t poseIndex_ = 0;
  uint16_t capturedSamples_ = 0;
  uint32_t lastSampleMs_ = 0;
  uint32_t stableSinceMs_ = 0;
  uint32_t stateStartedMs_ = 0;
  uint32_t statusUntilMs_ = 0;
  const char* statusText_ = nullptr;
};

}  // namespace iris
