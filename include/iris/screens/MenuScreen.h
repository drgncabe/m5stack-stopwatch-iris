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
  void activateSelected();
  int rowAt(int32_t x, int32_t y) const;

  const char* title_;
  const MenuItem* items_;
  size_t itemCount_;
  SettingsStore& settings_;
  size_t selected_ = 0;
};

}  // namespace iris
