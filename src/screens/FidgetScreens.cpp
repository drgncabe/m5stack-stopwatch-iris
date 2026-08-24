#include "iris/screens/FidgetScreens.h"

#include <M5Unified.h>
#include <math.h>

#include "iris/Theme.h"
#include "iris/screens/ScreenManager.h"

namespace iris {

namespace {
constexpr float kPi = 3.1415926535f;
constexpr int kScreenCenter = 233;
constexpr int kPlayTop = 62;
constexpr int kPlayBottom = 410;
constexpr int kPlayLeft = 24;
constexpr int kPlayRight = 442;
constexpr int kPlayWidth = kPlayRight - kPlayLeft + 1;
constexpr int kPlayHeight = kPlayBottom - kPlayTop + 1;
constexpr uint32_t kFrameMs = 66;

float clampFloat(float value, float low, float high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

float wrapAngle(float angle) {
  while (angle > kPi) angle -= 2.0f * kPi;
  while (angle < -kPi) angle += 2.0f * kPi;
  return angle;
}

float distanceSquared(float ax, float ay, float bx, float by) {
  const float dx = ax - bx;
  const float dy = ay - by;
  return dx * dx + dy * dy;
}

int localX(float x) {
  return static_cast<int>(x) - kPlayLeft;
}

int localY(float y) {
  return static_cast<int>(y) - kPlayTop;
}

void readGravity(const SettingsStore& settings, float* gx, float* gy) {
  *gx = 0.0f;
  *gy = 260.0f;
  if (!M5.Imu.isEnabled()) return;

  M5.Imu.update();
  float ax = 0.0f;
  float ay = 0.0f;
  float az = 0.0f;
  if (!M5.Imu.getAccel(&ax, &ay, &az)) return;
  ax += settings.accelOffsetX();
  ay += settings.accelOffsetY();
  az += settings.accelOffsetZ();

  const float rawX = ax * 360.0f;
  const float rawY = -ay * 360.0f;
  switch (M5.Display.getRotation() & 3) {
    case 1:
      *gx = -rawY;
      *gy = rawX;
      break;
    case 2:
      *gx = -rawX;
      *gy = -rawY;
      break;
    case 3:
      *gx = rawY;
      *gy = -rawX;
      break;
    default:
      *gx = rawX;
      *gy = rawY;
      break;
  }
}

}  // namespace

FidgetScreenBase::FidgetScreenBase(const char* title, SettingsStore& settings)
    : settings_(settings), title_(title), canvas_(&M5.Display) {}

void FidgetScreenBase::enter() {
  lastUpdateMs_ = millis();
  lastDrawMs_ = 0;
  dirty_ = true;
  chromeDirty_ = true;
  M5.Power.setVibration(0);
  if (!canvas_.getBuffer()) {
    canvas_.setPsram(true);
    canvas_.setColorDepth(16);
    canvasReady_ = canvas_.createSprite(kPlayWidth, kPlayHeight) != nullptr;
  }
  M5.Display.fillScreen(currentTheme(settings_).background);
  reset();
}

void FidgetScreenBase::update(uint32_t nowMs) {
  updateHaptic(nowMs);
  const float dt = clampFloat((nowMs - lastUpdateMs_) / 1000.0f, 0.0f, 0.08f);
  lastUpdateMs_ = nowMs;
  updateFidget(nowMs, dt);

  if (dirty_ || nowMs - lastDrawMs_ >= kFrameMs) {
    draw();
    lastDrawMs_ = nowMs;
    dirty_ = false;
  }
}

void FidgetScreenBase::draw() {
  const Theme theme = currentTheme(settings_);
  if (chromeDirty_) {
    M5.Display.fillScreen(theme.background);
    drawChrome();
    chromeDirty_ = false;
  }
  if (!canvasReady_) return;

  canvas().fillScreen(theme.background);
  drawFidget();
  canvas().pushSprite(kPlayLeft, kPlayTop);
}

void FidgetScreenBase::onButtonA() {
  M5.Power.setVibration(0);
  if (manager_) manager_->show(ScreenId::Fidgets);
}

void FidgetScreenBase::onButtonB() {
  M5.Power.setVibration(0);
  if (manager_) manager_->show(ScreenId::Fidgets);
}

void FidgetScreenBase::drawChrome() {
  const Theme theme = currentTheme(settings_);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSansBold12pt7b);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.drawString(title_, M5.Display.width() / 2, 34);

  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(theme.muted, theme.background);
  M5.Display.drawString("A/B: Back", M5.Display.width() / 2, 432);
}

void FidgetScreenBase::pulseHaptic(uint8_t strength, uint32_t durationMs) {
  M5.Power.setVibration(strength);
  hapticActive_ = true;
  hapticOffMs_ = millis() + durationMs;
}

void FidgetScreenBase::updateHaptic(uint32_t nowMs) {
  if (!hapticActive_ || nowMs < hapticOffMs_) return;
  M5.Power.setVibration(0);
  hapticActive_ = false;
}

WheelFidgetScreen::WheelFidgetScreen(SettingsStore& settings)
    : FidgetScreenBase("Wheel", settings) {}

void WheelFidgetScreen::reset() {
  x_ = kScreenCenter;
  y_ = kScreenCenter;
  angle_ = 0.0f;
  lastHapticMs_ = 0;
}

void WheelFidgetScreen::previewTouch(int32_t x, int32_t y) {
  moveWheel(x, y, true);
}

void WheelFidgetScreen::handleTouch(int32_t x, int32_t y) {
  moveWheel(x, y, true);
}

void WheelFidgetScreen::updateFidget(uint32_t nowMs, float dt) {
  angle_ += dt * 1.2f;
}

void WheelFidgetScreen::drawFidget() {
  const Theme theme = currentTheme(settings_);
  constexpr int radius = 58;
  canvas().fillCircle(localX(x_), localY(y_), radius + 8, theme.panel);
  canvas().fillCircle(localX(x_), localY(y_), radius, theme.button);
  canvas().drawCircle(localX(x_), localY(y_), radius, theme.foreground);

  for (int i = 0; i < 12; ++i) {
    const float a = angle_ + (2.0f * kPi * i / 12.0f);
    const int x1 = static_cast<int>(x_ + cosf(a) * 14.0f);
    const int y1 = static_cast<int>(y_ + sinf(a) * 14.0f);
    const int x2 = static_cast<int>(x_ + cosf(a) * 52.0f);
    const int y2 = static_cast<int>(y_ + sinf(a) * 52.0f);
    canvas().drawLine(localX(x1), localY(y1), localX(x2), localY(y2), theme.foreground);
  }
  canvas().fillCircle(localX(x_), localY(y_), 14, theme.accent);
}

void WheelFidgetScreen::moveWheel(int32_t x, int32_t y, bool feedback) {
  constexpr float radius = 66.0f;
  const float nextX = clampFloat(static_cast<float>(x), kPlayLeft + radius, kPlayRight - radius);
  const float nextY = clampFloat(static_cast<float>(y), kPlayTop + radius, kPlayBottom - radius);
  const float distSq = distanceSquared(x_, y_, nextX, nextY);
  if (distSq < 9.0f) return;

  angle_ += sqrtf(distSq) * 0.035f;
  x_ = nextX;
  y_ = nextY;
  const uint32_t nowMs = millis();
  if (feedback && nowMs - lastHapticMs_ >= 34) {
    pulseHaptic(80, 10);
    lastHapticMs_ = nowMs;
  }
  requestDraw();
}

PoppersFidgetScreen::PoppersFidgetScreen(SettingsStore& settings)
    : FidgetScreenBase("Poppers", settings) {}

void PoppersFidgetScreen::reset() {
  lastPopAttemptMs_ = 0;
  constexpr uint16_t colors[] = {0xF81F, 0x07FF, 0xFFE0, 0xFD20, 0x07E0, 0x001F,
                                 0xF800, 0xAFE5, 0xFC9F};
  for (size_t i = 0; i < kBallCount; ++i) {
    const float col = static_cast<float>(i % 3);
    const float row = static_cast<float>(i / 3);
    balls_[i] = {
        120.0f + col * 112.0f,
        118.0f + row * 82.0f,
        (static_cast<int>(i % 2) == 0 ? 36.0f : -28.0f),
        (static_cast<int>(i % 3) - 1) * 24.0f,
        22.0f + static_cast<float>((i % 3) * 3),
        colors[i],
        false,
        0,
    };
  }
}

void PoppersFidgetScreen::previewTouch(int32_t x, int32_t y) {
  popAt(x, y);
}

void PoppersFidgetScreen::handleTouch(int32_t x, int32_t y) {
  popAt(x, y);
}

void PoppersFidgetScreen::updateFidget(uint32_t nowMs, float dt) {
  float gx = 0.0f;
  float gy = 0.0f;
  readGravity(settings_, &gx, &gy);

  for (size_t i = 0; i < kBallCount; ++i) {
    Ball& ball = balls_[i];
    if (ball.popped) {
      if (nowMs - ball.poppedAtMs >= 1300) {
        ball.popped = false;
        ball.x = kScreenCenter + cosf(i) * 44.0f;
        ball.y = 176.0f + sinf(i * 1.7f) * 34.0f;
        ball.vx = cosf(i * 2.1f) * 80.0f;
        ball.vy = sinf(i * 1.3f) * 70.0f;
      }
      requestDraw();
      continue;
    }

    ball.vx += gx * dt;
    ball.vy += gy * dt;
    ball.vx *= 0.992f;
    ball.vy *= 0.992f;
    ball.x += ball.vx * dt;
    ball.y += ball.vy * dt;

    const float left = kPlayLeft + ball.radius;
    const float right = kPlayRight - ball.radius;
    const float top = kPlayTop + ball.radius;
    const float bottom = kPlayBottom - ball.radius;
    if (ball.x < left) {
      ball.x = left;
      ball.vx *= -0.68f;
    } else if (ball.x > right) {
      ball.x = right;
      ball.vx *= -0.68f;
    }
    if (ball.y < top) {
      ball.y = top;
      ball.vy *= -0.68f;
    } else if (ball.y > bottom) {
      ball.y = bottom;
      ball.vy *= -0.68f;
    }
  }
}

void PoppersFidgetScreen::drawFidget() {
  const Theme theme = currentTheme(settings_);
  canvas().drawCircle(localX(kScreenCenter), localY(236), 188, theme.panel);
  for (size_t i = 0; i < kBallCount; ++i) {
    const Ball& ball = balls_[i];
    const int x = localX(ball.x);
    const int y = localY(ball.y);
    if (ball.popped) {
      canvas().drawCircle(x, y, static_cast<int>(ball.radius + 8), ball.color);
      canvas().drawCircle(x, y, static_cast<int>(ball.radius + 16), theme.muted);
      continue;
    }
    canvas().fillCircle(x + 4, y + 6, static_cast<int>(ball.radius), theme.panel);
    canvas().fillCircle(x, y, static_cast<int>(ball.radius), ball.color);
    canvas().fillCircle(x - 7, y - 8, 6, 0xFFFF);
  }
}

void PoppersFidgetScreen::popAt(int32_t x, int32_t y) {
  const uint32_t nowMs = millis();
  if (nowMs - lastPopAttemptMs_ < 60) return;
  lastPopAttemptMs_ = nowMs;
  for (size_t i = 0; i < kBallCount; ++i) {
    Ball& ball = balls_[i];
    if (ball.popped) continue;
    const float hit = ball.radius + 24.0f;
    if (distanceSquared(ball.x, ball.y, x, y) <= hit * hit) {
      ball.popped = true;
      ball.poppedAtMs = nowMs;
      pulseHaptic(96, 18);
      requestDraw();
      return;
    }
  }
}

SpinnerFidgetScreen::SpinnerFidgetScreen(SettingsStore& settings)
    : FidgetScreenBase("Kaleidoscope", settings) {}

void SpinnerFidgetScreen::reset() {
  angle_ = 0.0f;
  angularVelocity_ = 1.6f;
  trackingTouch_ = false;
  lastTouchMs_ = 0;
}

void SpinnerFidgetScreen::previewTouch(int32_t x, int32_t y) {
  const uint32_t nowMs = millis();
  const float nextAngle = touchAngle(x, y);
  if (trackingTouch_ && lastTouchMs_ != 0) {
    const float dt = clampFloat((nowMs - lastTouchMs_) / 1000.0f, 0.01f, 0.12f);
    const float delta = wrapAngle(nextAngle - lastTouchAngle_);
    angle_ += delta;
    angularVelocity_ = delta / dt;
  }
  trackingTouch_ = true;
  lastTouchAngle_ = nextAngle;
  lastTouchMs_ = nowMs;
  requestDraw();
}

void SpinnerFidgetScreen::handleTouch(int32_t x, int32_t y) {
  previewTouch(x, y);
  trackingTouch_ = false;
}

void SpinnerFidgetScreen::updateFidget(uint32_t nowMs, float dt) {
  angle_ += angularVelocity_ * dt;
  angularVelocity_ *= 0.992f;
}

void SpinnerFidgetScreen::drawFidget() {
  constexpr uint16_t colors[] = {0xF81F, 0x07FF, 0xFFE0, 0xFD20, 0xAFE5, 0xF800};
  constexpr int arms = 18;
  constexpr float radius = 150.0f;
  for (int i = 0; i < arms; ++i) {
    const float a1 = angle_ + (2.0f * kPi * i / arms);
    const float a2 = angle_ + (2.0f * kPi * (i + 1) / arms);
    const int x1 = static_cast<int>(kScreenCenter + cosf(a1) * radius);
    const int y1 = static_cast<int>(236 + sinf(a1) * radius);
    const int x2 = static_cast<int>(kScreenCenter + cosf(a2) * radius * 0.62f);
    const int y2 = static_cast<int>(236 + sinf(a2) * radius * 0.62f);
    canvas().fillTriangle(localX(kScreenCenter), localY(236), localX(x1), localY(y1),
                          localX(x2), localY(y2), colors[i % 6]);
  }
  canvas().fillCircle(localX(kScreenCenter), localY(236), 42, 0x0000);
  canvas().drawCircle(localX(kScreenCenter), localY(236), 154, 0xFFFF);
  canvas().drawCircle(localX(kScreenCenter), localY(236), 88, 0xFFFF);
  canvas().fillCircle(localX(kScreenCenter), localY(236), 18, 0xFFFF);
}

float SpinnerFidgetScreen::touchAngle(int32_t x, int32_t y) const {
  return atan2f(static_cast<float>(y - 236), static_cast<float>(x - kScreenCenter));
}

GravityBallFidgetScreen::GravityBallFidgetScreen(SettingsStore& settings)
    : FidgetScreenBase("Gravity ball", settings) {}

void GravityBallFidgetScreen::reset() {
  x_ = kScreenCenter;
  y_ = 190.0f;
  vx_ = 90.0f;
  vy_ = 0.0f;
}

void GravityBallFidgetScreen::previewTouch(int32_t x, int32_t y) {
  x_ = clampFloat(x, 60.0f, 406.0f);
  y_ = clampFloat(y, 80.0f, 390.0f);
  vx_ = 0.0f;
  vy_ = 0.0f;
  requestDraw();
}

void GravityBallFidgetScreen::handleTouch(int32_t x, int32_t y) {
  previewTouch(x, y);
}

void GravityBallFidgetScreen::updateFidget(uint32_t nowMs, float dt) {
  float gx = 0.0f;
  float gy = 0.0f;
  readGravity(settings_, &gx, &gy);

  vx_ += gx * dt;
  vy_ += gy * dt;
  vx_ *= 0.996f;
  vy_ *= 0.996f;
  x_ += vx_ * dt;
  y_ += vy_ * dt;

  constexpr float ballRadius = 26.0f;
  constexpr float arenaRadius = 174.0f;
  constexpr float centerY = 236.0f;
  const float dx = x_ - kScreenCenter;
  const float dy = y_ - centerY;
  const float dist = sqrtf(dx * dx + dy * dy);
  const float maxDist = arenaRadius - ballRadius;
  if (dist > maxDist && dist > 0.001f) {
    const float nx = dx / dist;
    const float ny = dy / dist;
    x_ = kScreenCenter + nx * maxDist;
    y_ = centerY + ny * maxDist;

    const float velocityAlongNormal = vx_ * nx + vy_ * ny;
    if (velocityAlongNormal > 0.0f) {
      vx_ -= 1.78f * velocityAlongNormal * nx;
      vy_ -= 1.78f * velocityAlongNormal * ny;
    }
  }
}

void GravityBallFidgetScreen::drawFidget() {
  const Theme theme = currentTheme(settings_);
  canvas().drawCircle(localX(kScreenCenter), localY(236), 174, theme.panel);
  canvas().drawCircle(localX(kScreenCenter), localY(236), 175, theme.muted);
  canvas().fillCircle(localX(x_ + 6), localY(y_ + 8), 26, theme.panel);
  canvas().fillCircle(localX(x_), localY(y_), 26, theme.accent);
  canvas().fillCircle(localX(x_ - 8), localY(y_ - 9), 7, 0xFFFF);
}

}  // namespace iris
