#include "iris/screens/StopwatchScreens.h"

#include <M5Unified.h>
#include <esp_timer.h>
#include <math.h>

#include "iris/Theme.h"
#include "iris/screens/ScreenManager.h"

namespace iris {

namespace {
constexpr float kPi = 3.1415926535f;
constexpr int kCenter = 233;
constexpr uint32_t kRunningFrameMs = 50;
constexpr uint32_t kPausedFrameMs = 250;
constexpr uint64_t kMeaningfulSessionUs = 2ULL * 1000ULL * 1000ULL;

uint16_t dimColor(uint16_t color) {
  const uint8_t r = (color >> 11) & 0x1F;
  const uint8_t g = (color >> 5) & 0x3F;
  const uint8_t b = color & 0x1F;
  return static_cast<uint16_t>(((r / 2) << 11) | ((g / 2) << 5) | (b / 2));
}

}  // namespace

void StopwatchEngine::start(uint64_t nowUs) {
  if (state_ != StopwatchRunState::Reset) return;
  accumulatedUs_ = 0;
  runningStartedUs_ = nowUs;
  currentLapStartedTotalUs_ = 0;
  lapCount_ = 0;
  nextLapNumber_ = 1;
  state_ = StopwatchRunState::Running;
}

void StopwatchEngine::pause(uint64_t nowUs) {
  if (state_ != StopwatchRunState::Running) return;
  accumulatedUs_ = elapsedUs(nowUs);
  runningStartedUs_ = 0;
  state_ = StopwatchRunState::Paused;
}

void StopwatchEngine::resume(uint64_t nowUs) {
  if (state_ != StopwatchRunState::Paused) return;
  runningStartedUs_ = nowUs;
  state_ = StopwatchRunState::Running;
}

void StopwatchEngine::reset() {
  state_ = StopwatchRunState::Reset;
  accumulatedUs_ = 0;
  runningStartedUs_ = 0;
  currentLapStartedTotalUs_ = 0;
  lapCount_ = 0;
  nextLapNumber_ = 1;
}

bool StopwatchEngine::lap(uint64_t nowUs) {
  if (state_ != StopwatchRunState::Running) return false;

  const uint64_t total = elapsedUs(nowUs);
  const uint64_t lapDuration = total - currentLapStartedTotalUs_;
  currentLapStartedTotalUs_ = total;

  if (lapCount_ < kMaxLaps) {
    for (size_t i = lapCount_; i > 0; --i) {
      laps_[i] = laps_[i - 1];
    }
    lapCount_++;
  } else {
    for (size_t i = kMaxLaps - 1; i > 0; --i) {
      laps_[i] = laps_[i - 1];
    }
  }

  laps_[0].number = nextLapNumber_++;
  laps_[0].lapDurationUs = lapDuration;
  laps_[0].totalDurationUs = total;
  return true;
}

uint64_t StopwatchEngine::elapsedUs(uint64_t nowUs) const {
  if (state_ != StopwatchRunState::Running) return accumulatedUs_;
  return accumulatedUs_ + (nowUs - runningStartedUs_);
}

uint64_t StopwatchEngine::currentLapUs(uint64_t nowUs) const {
  const uint64_t total = elapsedUs(nowUs);
  return total >= currentLapStartedTotalUs_ ? total - currentLapStartedTotalUs_ : 0;
}

const StopwatchLapRecord* StopwatchEngine::lapAt(size_t indexFromNewest) const {
  return indexFromNewest < lapCount_ ? &laps_[indexFromNewest] : nullptr;
}

uint16_t StopwatchEngine::fastestLapNumber() const {
  if (lapCount_ < 2) return 0;
  uint64_t fastest = UINT64_MAX;
  uint16_t lapNumber = 0;
  for (size_t i = 0; i < lapCount_; ++i) {
    if (laps_[i].lapDurationUs < fastest) {
      fastest = laps_[i].lapDurationUs;
      lapNumber = laps_[i].number;
    }
  }
  return lapNumber;
}

uint16_t StopwatchEngine::slowestLapNumber() const {
  if (lapCount_ < 2) return 0;
  uint64_t slowest = 0;
  uint16_t lapNumber = 0;
  for (size_t i = 0; i < lapCount_; ++i) {
    if (laps_[i].lapDurationUs > slowest) {
      slowest = laps_[i].lapDurationUs;
      lapNumber = laps_[i].number;
    }
  }
  return lapNumber;
}

bool StopwatchEngine::hasMeaningfulSession(uint64_t nowUs) const {
  return lapCount_ > 0 || elapsedUs(nowUs) >= kMeaningfulSessionUs;
}

StopwatchScreen::StopwatchScreen(SettingsStore& settings, StopwatchEngine& engine)
    : settings_(settings), engine_(engine), canvas_(&M5.Display) {}

void StopwatchScreen::enter() {
  lastDrawMs_ = 0;
  previewAction_ = TouchAction::None;
  if (!canvas_.getBuffer()) {
    canvas_.setPsram(true);
    canvas_.setColorDepth(16);
    canvasReady_ = canvas_.createSprite(M5.Display.width(), M5.Display.height()) != nullptr;
  }
  draw();
}

void StopwatchScreen::update(uint32_t nowMs) {
  updateFeedback(nowMs);
  const uint32_t interval =
      engine_.state() == StopwatchRunState::Running ? kRunningFrameMs : kPausedFrameMs;
  if (lastDrawMs_ == 0 || nowMs - lastDrawMs_ >= interval) {
    draw();
    lastDrawMs_ = nowMs;
  }
}

void StopwatchScreen::draw() {
  const uint64_t elapsed = engine_.elapsedUs(nowUs());
  drawFace(elapsed, engine_.currentLapUs(nowUs()));
}

void StopwatchScreen::previewTouch(int32_t x, int32_t y) {
  const TouchAction action = actionAt(x, y);
  if (action == previewAction_) return;
  previewAction_ = action;
  draw();
}

void StopwatchScreen::handleTouch(int32_t x, int32_t y) {
  const TouchAction action = actionAt(x, y);
  previewAction_ = TouchAction::None;
  if (action == TouchAction::Left) {
    doLeftAction();
  } else if (action == TouchAction::Right) {
    doRightAction();
  } else if (action == TouchAction::Laps && engine_.lapCount() > 0 && manager_) {
    manager_->show(ScreenId::StopwatchLaps);
  } else {
    draw();
  }
}

void StopwatchScreen::onButtonA() {
  doLeftAction();
}

void StopwatchScreen::onButtonB() {
  doRightAction();
}

void StopwatchScreen::cancelFeedback() {
  if (!feedbackActive_) return;
  M5.Power.setVibration(0);
  feedbackActive_ = false;
}

uint64_t StopwatchScreen::nowUs() const {
  return static_cast<uint64_t>(esp_timer_get_time());
}

void StopwatchScreen::drawFace(uint64_t elapsedUs, uint64_t lapUs) {
  const Theme theme = currentTheme(settings_);
  if (!canvasReady_) {
    M5.Display.fillScreen(theme.background);
    return;
  }

  canvas_.fillScreen(theme.background);
  drawProgress(theme, elapsedUs);

  canvas_.setTextDatum(middle_center);
  canvas_.setFont(&fonts::FreeSans9pt7b);
  canvas_.setTextColor(theme.muted, theme.background);
  canvas_.drawString("STOPWATCH", kCenter, 54);

  drawTimePair(theme, elapsedUs);

  canvas_.setFont(&fonts::FreeSans12pt7b);
  canvas_.setTextColor(theme.accent, theme.background);
  const char* status = "READY";
  if (confirmReset_) {
    status = "RESET?";
  } else if (engine_.state() == StopwatchRunState::Running) {
    status = "RUNNING";
  } else if (engine_.state() == StopwatchRunState::Paused) {
    status = "PAUSED";
  }
  canvas_.drawString(status, kCenter, 278);

  char lapText[32];
  if (engine_.lapCount() > 0) {
    snprintf(lapText, sizeof(lapText), "LAP %02u", static_cast<unsigned>(engine_.lapCount()));
  } else {
    snprintf(lapText, sizeof(lapText), "NO LAPS");
  }
  canvas_.setFont(&fonts::FreeSans9pt7b);
  canvas_.setTextColor(engine_.lapCount() > 0 ? theme.foreground : theme.muted, theme.background);
  canvas_.drawString(lapText, kCenter, 314);

  if (engine_.state() != StopwatchRunState::Reset || engine_.lapCount() > 0) {
    char lapDuration[24];
    formatCompactTime(lapUs, lapDuration, sizeof(lapDuration));
    canvas_.setTextColor(theme.muted, theme.background);
    canvas_.drawString(String("Current ") + lapDuration, kCenter, 342);
  }

  if (confirmReset_) {
    drawResetConfirmation(theme, elapsedUs);
  } else {
    drawControls(theme);
  }

  canvas_.pushSprite(0, 0);
}

void StopwatchScreen::drawProgress(const Theme& theme, uint64_t elapsedUs) {
  constexpr int tickCount = 80;
  constexpr int outer = 210;
  constexpr int inner = 188;
  const uint32_t secondUs = static_cast<uint32_t>(elapsedUs % 1000000ULL);
  const int litTicks = static_cast<int>((static_cast<uint64_t>(secondUs) * tickCount) / 1000000ULL);
  const uint16_t idle = dimColor(theme.panel);
  const uint16_t active = engine_.state() == StopwatchRunState::Paused ? theme.muted : theme.accent;

  for (int i = 0; i < tickCount; ++i) {
    const float a = -kPi / 2.0f + (2.0f * kPi * i / tickCount);
    const int x1 = static_cast<int>(kCenter + cosf(a) * inner);
    const int y1 = static_cast<int>(kCenter + sinf(a) * inner);
    const int x2 = static_cast<int>(kCenter + cosf(a) * outer);
    const int y2 = static_cast<int>(kCenter + sinf(a) * outer);
    const uint16_t color = i <= litTicks ? active : idle;
    canvas_.drawLine(x1, y1, x2, y2, color);
  }
  canvas_.drawCircle(kCenter, kCenter, 176, theme.panel);
  canvas_.drawCircle(kCenter, kCenter, 177, dimColor(theme.muted));
}

void StopwatchScreen::drawControls(const Theme& theme) {
  const bool leftPreview = previewAction_ == TouchAction::Left;
  const bool rightPreview = previewAction_ == TouchAction::Right;
  const char* left = "LAPS";
  const char* right = "START";

  if (engine_.state() == StopwatchRunState::Running) {
    left = "LAP";
    right = "PAUSE";
  } else if (engine_.state() == StopwatchRunState::Paused) {
    left = "RESET";
    right = "RESUME";
  } else if (engine_.lapCount() == 0) {
    left = "BACK";
  }

  drawControlButton(48, 376, 168, 48, left, leftPreview ? theme.selected : theme.panel,
                    theme.muted, theme.foreground);
  drawControlButton(250, 376, 168, 48, right, rightPreview ? theme.selected : theme.accent,
                    theme.accent, theme.foreground);

  canvas_.setFont(&fonts::FreeSans9pt7b);
  canvas_.setTextColor(theme.muted, theme.background);
  canvas_.drawString(String("A: ") + left + "   B: " + right, kCenter, 444);
}

void StopwatchScreen::drawControlButton(int x, int y, int w, int h, const char* label,
                                        uint16_t fill, uint16_t outline, uint16_t text) {
  canvas_.fillRoundRect(x, y, w, h, 18, fill);
  canvas_.drawRoundRect(x, y, w, h, 18, outline);
  canvas_.setFont(&fonts::FreeSans9pt7b);
  canvas_.setTextDatum(middle_center);
  canvas_.setTextColor(text, fill);
  canvas_.drawString(label, x + w / 2, y + h / 2 + 1);
}

void StopwatchScreen::drawResetConfirmation(const Theme& theme, uint64_t elapsedUs) {
  char compact[24];
  formatCompactTime(elapsedUs, compact, sizeof(compact));
  canvas_.fillRoundRect(76, 320, 314, 104, 18, theme.panel);
  canvas_.drawRoundRect(76, 320, 314, 104, 18, theme.accent);
  canvas_.setTextDatum(middle_center);
  canvas_.setFont(&fonts::FreeSans9pt7b);
  canvas_.setTextColor(theme.foreground, theme.panel);
  canvas_.drawString(String(compact) + "  " + String(engine_.lapCount()) + " laps", kCenter, 344);

  drawControlButton(94, 372, 124, 42, "CANCEL",
                    previewAction_ == TouchAction::Left ? theme.selected : theme.background,
                    theme.muted, theme.foreground);
  drawControlButton(248, 372, 124, 42, "RESET",
                    previewAction_ == TouchAction::Right ? theme.selected : theme.accent,
                    theme.accent, theme.foreground);

  canvas_.setFont(&fonts::FreeSans9pt7b);
  canvas_.setTextColor(theme.muted, theme.background);
  canvas_.drawString("A: Cancel   B: Reset", kCenter, 444);
}

void StopwatchScreen::drawTimePair(const Theme& theme, uint64_t elapsedUs) {
  char mainText[24];
  char hundredthsText[8];
  formatMainTime(elapsedUs, mainText, sizeof(mainText), hundredthsText, sizeof(hundredthsText));

  canvas_.setTextDatum(middle_center);
  canvas_.setFont(&fonts::FreeSansBold24pt7b);
  canvas_.setTextColor(theme.foreground, theme.background);
  canvas_.drawString(mainText, kCenter, 176);

  canvas_.setFont(&fonts::FreeSansBold18pt7b);
  canvas_.setTextColor(theme.accent, theme.background);
  canvas_.drawString(hundredthsText, kCenter, 232);
}

void StopwatchScreen::formatMainTime(uint64_t elapsedUs, char* mainText, size_t mainSize,
                                     char* hundredthsText, size_t hundredthsSize) const {
  const uint64_t totalHundredths = elapsedUs / 10000ULL;
  const uint32_t hundredths = totalHundredths % 100ULL;
  const uint64_t totalSeconds = totalHundredths / 100ULL;
  const uint32_t seconds = totalSeconds % 60ULL;
  const uint32_t minutes = (totalSeconds / 60ULL) % 60ULL;
  const uint32_t hours = totalSeconds / 3600ULL;

  snprintf(mainText, mainSize, "%02lu:%02lu:%02lu", static_cast<unsigned long>(hours),
           static_cast<unsigned long>(minutes), static_cast<unsigned long>(seconds));
  snprintf(hundredthsText, hundredthsSize, ".%02lu", static_cast<unsigned long>(hundredths));
}

void StopwatchScreen::formatCompactTime(uint64_t elapsedUs, char* text, size_t textSize) const {
  const uint64_t totalHundredths = elapsedUs / 10000ULL;
  const uint32_t hundredths = totalHundredths % 100ULL;
  const uint64_t totalSeconds = totalHundredths / 100ULL;
  const uint32_t seconds = totalSeconds % 60ULL;
  const uint32_t minutes = (totalSeconds / 60ULL) % 60ULL;
  const uint32_t hours = totalSeconds / 3600ULL;
  if (hours > 0) {
    snprintf(text, textSize, "%02lu:%02lu:%02lu.%02lu", static_cast<unsigned long>(hours),
             static_cast<unsigned long>(minutes), static_cast<unsigned long>(seconds),
             static_cast<unsigned long>(hundredths));
  } else {
    snprintf(text, textSize, "%02lu:%02lu.%02lu", static_cast<unsigned long>(minutes),
             static_cast<unsigned long>(seconds), static_cast<unsigned long>(hundredths));
  }
}

void StopwatchScreen::doLeftAction() {
  if (confirmReset_) {
    cancelReset();
    return;
  }

  if (engine_.state() == StopwatchRunState::Running) {
    if (engine_.lap(nowUs())) pulse(96, 18);
  } else if (engine_.state() == StopwatchRunState::Paused) {
    if (engine_.hasMeaningfulSession(nowUs())) {
      confirmReset_ = true;
      pulse(64, 12);
    } else {
      engine_.reset();
      pulse(80, 16);
    }
  } else if (engine_.lapCount() > 0 && manager_) {
    manager_->show(ScreenId::StopwatchLaps);
    return;
  } else if (manager_) {
    manager_->show(ScreenId::MainMenu);
    return;
  }
  draw();
}

void StopwatchScreen::doRightAction() {
  const uint64_t now = nowUs();
  if (confirmReset_) {
    confirmReset();
    return;
  }

  if (engine_.state() == StopwatchRunState::Reset) {
    engine_.start(now);
    pulse(72, 14);
  } else if (engine_.state() == StopwatchRunState::Running) {
    engine_.pause(now);
    pulse(54, 18);
  } else {
    engine_.resume(now);
    pulse(72, 14);
  }
  draw();
}

void StopwatchScreen::confirmReset() {
  engine_.reset();
  confirmReset_ = false;
  pulse(110, 24);
  draw();
}

void StopwatchScreen::cancelReset() {
  confirmReset_ = false;
  pulse(48, 10);
  draw();
}

void StopwatchScreen::pulse(uint8_t strength, uint32_t durationMs) {
  M5.Power.setVibration(strength);
  feedbackActive_ = true;
  lastFeedbackOffMs_ = millis() + durationMs;
  if (settings_.volume() > 0) {
    M5.Speaker.tone(2400, 14);
  }
}

void StopwatchScreen::updateFeedback(uint32_t nowMs) {
  if (!feedbackActive_ || nowMs < lastFeedbackOffMs_) return;
  M5.Power.setVibration(0);
  feedbackActive_ = false;
}

StopwatchScreen::TouchAction StopwatchScreen::actionAt(int32_t x, int32_t y) const {
  if (y >= 366) {
    return x < kCenter ? TouchAction::Left : TouchAction::Right;
  }
  if (y >= 292 && y <= 356) return TouchAction::Laps;
  return TouchAction::None;
}

StopwatchLapHistoryScreen::StopwatchLapHistoryScreen(SettingsStore& settings,
                                                     StopwatchEngine& engine)
    : settings_(settings), engine_(engine) {}

void StopwatchLapHistoryScreen::enter() {
  offset_ = 0;
  draw();
}

void StopwatchLapHistoryScreen::update(uint32_t) {}

void StopwatchLapHistoryScreen::draw() {
  const Theme theme = currentTheme(settings_);
  M5.Display.fillScreen(theme.background);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSansBold12pt7b);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.drawString("LAPS", kCenter, 44);

  if (engine_.lapCount() == 0) {
    M5.Display.setFont(&fonts::FreeSans12pt7b);
    M5.Display.setTextColor(theme.muted, theme.background);
    M5.Display.drawString("No laps yet", kCenter, 220);
  } else {
    const uint16_t fastest = engine_.fastestLapNumber();
    const uint16_t slowest = engine_.slowestLapNumber();
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    for (size_t row = 0; row < 5; ++row) {
      const StopwatchLapRecord* lap = engine_.lapAt(offset_ + row);
      if (!lap) break;
      const int y = 108 + static_cast<int>(row) * 52;
      const bool markedFast = fastest != 0 && lap->number == fastest;
      const bool markedSlow = slowest != 0 && lap->number == slowest;
      M5.Display.fillRoundRect(56, y - 21, 354, 40, 12, markedFast ? theme.selected : theme.panel);
      M5.Display.setTextDatum(middle_left);
      M5.Display.setTextColor(theme.foreground, markedFast ? theme.selected : theme.panel);
      char label[12];
      snprintf(label, sizeof(label), "%02u", lap->number);
      M5.Display.drawString(label, 82, y);
      char duration[24];
      formatDuration(lap->lapDurationUs, duration, sizeof(duration));
      M5.Display.drawString(duration, 150, y);
      if (markedFast || markedSlow) {
        M5.Display.setTextDatum(middle_right);
        M5.Display.setTextColor(markedFast ? theme.accent : theme.muted,
                                markedFast ? theme.selected : theme.panel);
        M5.Display.drawString(markedFast ? "FAST" : "SLOW", 382, y);
      }
    }
  }

  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(theme.muted, theme.background);
  M5.Display.drawString("A: Back   B: Scroll", kCenter, 432);
}

void StopwatchLapHistoryScreen::handleTouch(int32_t x, int32_t y) {
  if (y > 382 && manager_) {
    manager_->show(ScreenId::Stopwatch);
  } else if (y < kCenter) {
    scroll(-1);
  } else {
    scroll(1);
  }
}

void StopwatchLapHistoryScreen::onButtonA() {
  if (manager_) manager_->show(ScreenId::Stopwatch);
}

void StopwatchLapHistoryScreen::onButtonB() {
  scroll(1);
}

void StopwatchLapHistoryScreen::formatDuration(uint64_t elapsedUs, char* text,
                                               size_t textSize) const {
  const uint64_t totalHundredths = elapsedUs / 10000ULL;
  const uint32_t hundredths = totalHundredths % 100ULL;
  const uint64_t totalSeconds = totalHundredths / 100ULL;
  const uint32_t seconds = totalSeconds % 60ULL;
  const uint32_t minutes = (totalSeconds / 60ULL) % 60ULL;
  const uint32_t hours = totalSeconds / 3600ULL;
  if (hours > 0) {
    snprintf(text, textSize, "%02lu:%02lu:%02lu.%02lu", static_cast<unsigned long>(hours),
             static_cast<unsigned long>(minutes), static_cast<unsigned long>(seconds),
             static_cast<unsigned long>(hundredths));
  } else {
    snprintf(text, textSize, "%02lu:%02lu.%02lu", static_cast<unsigned long>(minutes),
             static_cast<unsigned long>(seconds), static_cast<unsigned long>(hundredths));
  }
}

void StopwatchLapHistoryScreen::scroll(int delta) {
  if (engine_.lapCount() <= 5) return;
  const size_t maxOffset = engine_.lapCount() - 5;
  if (delta < 0) {
    offset_ = offset_ == 0 ? maxOffset : offset_ - 1;
  } else {
    offset_ = offset_ >= maxOffset ? 0 : offset_ + 1;
  }
  draw();
}

}  // namespace iris
