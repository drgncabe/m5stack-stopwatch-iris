#pragma once

#include <Arduino.h>
#include <M5Unified.h>

#include "iris/screens/Screen.h"
#include "iris/services/SettingsStore.h"

namespace iris {

class FidgetScreenBase : public Screen {
 public:
  FidgetScreenBase(const char* title, SettingsStore& settings);

  void enter() override;
  void update(uint32_t nowMs) override;
  void draw() override;
  void handleTouch(int32_t x, int32_t y) override {}
  void onButtonA() override;
  void onButtonB() override;

 protected:
  virtual void reset() {}
  virtual void updateFidget(uint32_t nowMs, float dt) {}
  virtual void drawFidget() = 0;

  void requestDraw() { dirty_ = true; }
  void drawChrome();
  void pulseHaptic(uint8_t strength = 82, uint32_t durationMs = 12);
  M5Canvas& canvas() { return canvas_; }

  SettingsStore& settings_;
  const char* title_;
  bool dirty_ = true;
  bool chromeDirty_ = true;
  uint32_t lastUpdateMs_ = 0;
  uint32_t lastDrawMs_ = 0;
  M5Canvas canvas_;
  bool canvasReady_ = false;

 private:
  void updateHaptic(uint32_t nowMs);

  bool hapticActive_ = false;
  uint32_t hapticOffMs_ = 0;
};

class WheelFidgetScreen : public FidgetScreenBase {
 public:
  explicit WheelFidgetScreen(SettingsStore& settings);

  void previewTouch(int32_t x, int32_t y) override;
  void handleTouch(int32_t x, int32_t y) override;

 protected:
  void reset() override;
  void updateFidget(uint32_t nowMs, float dt) override;
  void drawFidget() override;

 private:
  void moveWheel(int32_t x, int32_t y, bool feedback);

  float x_ = 233.0f;
  float y_ = 233.0f;
  float angle_ = 0.0f;
  uint32_t lastHapticMs_ = 0;
};

class PoppersFidgetScreen : public FidgetScreenBase {
 public:
  explicit PoppersFidgetScreen(SettingsStore& settings);

  void previewTouch(int32_t x, int32_t y) override;
  void handleTouch(int32_t x, int32_t y) override;

 protected:
  void reset() override;
  void updateFidget(uint32_t nowMs, float dt) override;
  void drawFidget() override;

 private:
  struct Ball {
    float x;
    float y;
    float vx;
    float vy;
    float radius;
    uint16_t color;
    bool popped;
    uint32_t poppedAtMs;
  };

  void popAt(int32_t x, int32_t y);

  static constexpr size_t kBallCount = 9;
  Ball balls_[kBallCount]{};
  uint32_t lastPopAttemptMs_ = 0;
};

class SpinnerFidgetScreen : public FidgetScreenBase {
 public:
  explicit SpinnerFidgetScreen(SettingsStore& settings);

  void previewTouch(int32_t x, int32_t y) override;
  void handleTouch(int32_t x, int32_t y) override;

 protected:
  void reset() override;
  void updateFidget(uint32_t nowMs, float dt) override;
  void drawFidget() override;

 private:
  float touchAngle(int32_t x, int32_t y) const;

  float angle_ = 0.0f;
  float angularVelocity_ = 1.6f;
  float lastTouchAngle_ = 0.0f;
  uint32_t lastTouchMs_ = 0;
  bool trackingTouch_ = false;
};

class GravityBallFidgetScreen : public FidgetScreenBase {
 public:
  explicit GravityBallFidgetScreen(SettingsStore& settings);

  void previewTouch(int32_t x, int32_t y) override;
  void handleTouch(int32_t x, int32_t y) override;

 protected:
  void reset() override;
  void updateFidget(uint32_t nowMs, float dt) override;
  void drawFidget() override;

 private:
  float x_ = 233.0f;
  float y_ = 233.0f;
  float vx_ = 90.0f;
  float vy_ = 0.0f;
};

}  // namespace iris
