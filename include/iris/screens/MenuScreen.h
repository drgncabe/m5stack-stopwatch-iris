#pragma once

#include <Arduino.h>
#include "iris/screens/Screen.h"
#include "iris/services/SettingsStore.h"

namespace iris {

struct MenuItem {
  const char* label;
  ScreenId target;
};

class MenuScreen : public Screen {
 public:
  MenuScreen(const char* title, const MenuItem* items, size_t itemCount,
             SettingsStore& settings);

  void enter() override;
  void update(uint32_t nowMs) override;
  void draw() override;
  void previewTouch(int32_t x, int32_t y) override;
  void handleTouch(int32_t x, int32_t y) override;
  void onButtonA() override;
  void onButtonB() override;

 private:
  void drawRow(size_t index, bool selected);
  void selectRow(size_t row);
  void scrollSelection(int direction);
  void activateSelected();
  void startSelectionHaptic();
  void stopSelectionHaptic();

  const char* title_;
  const MenuItem* items_;
  size_t itemCount_;
  SettingsStore& settings_;
  size_t selected_ = 0;
  bool touchGestureActive_ = false;
  int32_t lastTouchY_ = 0;
  int32_t scrollRemainder_ = 0;
  bool hapticActive_ = false;
  uint32_t hapticOffMs_ = 0;
};

}  // namespace iris
