#include "iris/services/OrientationService.h"

#include <M5Unified.h>
#include <math.h>

namespace iris {

namespace {
constexpr uint32_t kSampleIntervalMs = 250;
constexpr uint32_t kSettleMs = 650;
constexpr float kMinAxisG = 0.38f;
constexpr float kFlatDominanceMargin = 0.12f;
}

void OrientationService::begin() {
  available_ = M5.Imu.isEnabled();
  rotation_ = M5.Display.getRotation() & 0x03;
  pendingRotation_ = -1;
  pendingSinceMs_ = 0;
  lastSampleMs_ = 0;
}

bool OrientationService::update(uint32_t nowMs, bool enabled) {
  if (!enabled || !available_) {
    pendingRotation_ = -1;
    pendingSinceMs_ = 0;
    return false;
  }
  if (lastSampleMs_ != 0 && nowMs - lastSampleMs_ < kSampleIntervalMs) return false;
  lastSampleMs_ = nowMs;

  M5.Imu.update();

  float ax = 0.0f;
  float ay = 0.0f;
  float az = 0.0f;
  if (!M5.Imu.getAccel(&ax, &ay, &az)) return false;

  const int8_t candidate = candidateRotation(ax, ay, az);
  if (candidate < 0 || candidate == rotation_) {
    pendingRotation_ = -1;
    pendingSinceMs_ = 0;
    return false;
  }

  if (candidate != pendingRotation_) {
    pendingRotation_ = candidate;
    pendingSinceMs_ = nowMs;
    return false;
  }

  if (nowMs - pendingSinceMs_ < kSettleMs) return false;

  applyRotation(static_cast<uint8_t>(candidate));
  pendingRotation_ = -1;
  pendingSinceMs_ = 0;
  return true;
}

const char* OrientationService::statusText() const {
  return available_ ? "Auto" : "Unavailable";
}

int8_t OrientationService::candidateRotation(float ax, float ay, float az) const {
  const float absX = fabsf(ax);
  const float absY = fabsf(ay);
  const float absZ = fabsf(az);
  const float dominantXY = absX > absY ? absX : absY;

  if (dominantXY < kMinAxisG) return -1;
  if (absZ > dominantXY + kFlatDominanceMargin) return -1;

  if (absX > absY) {
    return ax > 0.0f ? 3 : 1;
  }
  return ay > 0.0f ? 2 : 0;
}

void OrientationService::applyRotation(uint8_t rotation) {
  rotation_ = rotation & 0x03;
  M5.Display.setRotation(rotation_);
}

}  // namespace iris
