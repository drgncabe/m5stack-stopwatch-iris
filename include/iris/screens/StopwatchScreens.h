#pragma once

#include <Arduino.h>
#include <M5Unified.h>

#include "iris/screens/Screen.h"
#include "iris/services/SettingsStore.h"

namespace iris {

struct Theme;

enum class StopwatchRunState : uint8_t {
  Reset,
  Running,
  Paused,
};

struct StopwatchLapRecord {
  uint16_t number = 0;
  uint64_t lapDurationUs = 0;
  uint64_t totalDurationUs = 0;
};

class StopwatchEngine {
 public:
  static constexpr size_t kMaxLaps = 64;

  void start(uint64_t nowUs);
  void pause(uint64_t nowUs);
  void resume(uint64_t nowUs);
  void reset();
  bool lap(uint64_t nowUs);

  StopwatchRunState state() const { return state_; }
  uint64_t elapsedUs(uint64_t nowUs) const;
  uint64_t currentLapUs(uint64_t nowUs) const;
  size_t lapCount() const { return lapCount_; }
  const StopwatchLapRecord* lapAt(size_t indexFromNewest) const;
  uint16_t fastestLapNumber() const;
  uint16_t slowestLapNumber() const;
  bool hasMeaningfulSession(uint64_t nowUs) const;

 private:
  StopwatchRunState state_ = StopwatchRunState::Reset;
  uint64_t accumulatedUs_ = 0;
  uint64_t runningStartedUs_ = 0;
  uint64_t currentLapStartedTotalUs_ = 0;
  StopwatchLapRecord laps_[kMaxLaps]{};
  size_t lapCount_ = 0;
  uint16_t nextLapNumber_ = 1;
};

class StopwatchScreen : public Screen {
 public:
  StopwatchScreen(SettingsStore& settings, StopwatchEngine& engine);

  void enter() override;
  void update(uint32_t nowMs) override;
  void draw() override;
  void previewTouch(int32_t x, int32_t y) override;
  void handleTouch(int32_t x, int32_t y) override;
  void onButtonA() override;
  void onButtonB() override;
  void cancelFeedback();

 private:
  enum class TouchAction : uint8_t {
    None,
    Left,
    Right,
    Laps,
  };

  uint64_t nowUs() const;
  void drawFace(uint64_t elapsedUs, uint64_t lapUs);
  void drawProgress(const Theme& theme, uint64_t elapsedUs);
  void drawControls(const Theme& theme);
  void drawControlButton(int x, int y, int w, int h, const char* label, uint16_t fill,
                         uint16_t outline, uint16_t text);
  void drawResetConfirmation(const Theme& theme, uint64_t elapsedUs);
  void drawTimePair(const Theme& theme, uint64_t elapsedUs);
  void formatMainTime(uint64_t elapsedUs, char* mainText, size_t mainSize, char* hundredthsText,
                      size_t hundredthsSize) const;
  void formatCompactTime(uint64_t elapsedUs, char* text, size_t textSize) const;
  void doLeftAction();
  void doRightAction();
  void confirmReset();
  void cancelReset();
  void pulse(uint8_t strength, uint32_t durationMs);
  void updateFeedback(uint32_t nowMs);
  TouchAction actionAt(int32_t x, int32_t y) const;

  SettingsStore& settings_;
  StopwatchEngine& engine_;
  M5Canvas canvas_;
  bool canvasReady_ = false;
  uint32_t lastDrawMs_ = 0;
  uint32_t lastFeedbackOffMs_ = 0;
  bool feedbackActive_ = false;
  bool confirmReset_ = false;
  TouchAction previewAction_ = TouchAction::None;
};

class StopwatchLapHistoryScreen : public Screen {
 public:
  StopwatchLapHistoryScreen(SettingsStore& settings, StopwatchEngine& engine);

  void enter() override;
  void update(uint32_t nowMs) override;
  void draw() override;
  void handleTouch(int32_t x, int32_t y) override;
  void onButtonA() override;
  void onButtonB() override;

 private:
  void formatDuration(uint64_t elapsedUs, char* text, size_t textSize) const;
  void scroll(int delta);

  SettingsStore& settings_;
  StopwatchEngine& engine_;
  size_t offset_ = 0;
};

}  // namespace iris
