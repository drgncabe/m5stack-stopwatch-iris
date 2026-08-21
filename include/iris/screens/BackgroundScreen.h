#pragma once

#include "iris/screens/Screen.h"
#include "iris/services/SettingsStore.h"

namespace iris {

class BackgroundScreen : public Screen {
 public:
  explicit BackgroundScreen(SettingsStore& settings) : settings_(settings) {}

  void enter() override;
  void update(uint32_t nowMs) override;
  void draw() override;
  void previewTouch(int32_t x, int32_t y) override;
  void handleTouch(int32_t x, int32_t y) override;
  void onButtonA() override;
  void onButtonB() override;

 private:
  void activateSelected();
  void selectRow(size_t index);
  void drawRow(size_t index, bool selected);
  int rowAt(int32_t x, int32_t y) const;
  void nextTheme();
  void toggleWidget(uint8_t widget);
  void goBack();
  const char* rowLabel(size_t index) const;
  String rowValue(size_t index) const;

  SettingsStore& settings_;
  size_t selected_ = 0;
};

}  // namespace iris
