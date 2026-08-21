#pragma once

#include <Arduino.h>

namespace iris {

class OrientationService {
 public:
  void begin();
  bool update(uint32_t nowMs, bool enabled);

  bool available() const { return available_; }
  uint8_t rotation() const { return rotation_; }
  const char* statusText() const;

 private:
  int8_t candidateRotation(float ax, float ay, float az) const;
  void applyRotation(uint8_t rotation);

  bool available_ = false;
  uint8_t rotation_ = 0;
  int8_t pendingRotation_ = -1;
  uint32_t lastSampleMs_ = 0;
  uint32_t pendingSinceMs_ = 0;
};

}  // namespace iris
