#pragma once

#include "iris/screens/Screen.h"
#include "iris/services/SettingsStore.h"
#include "iris/services/WifiService.h"

namespace iris {

class WifiScreen : public Screen {
 public:
  WifiScreen(SettingsStore& settings, WifiService& wifi)
      : settings_(settings), wifi_(wifi) {}

  void enter() override;
  void update(uint32_t nowMs) override;
  void draw() override;
  void previewTouch(int32_t x, int32_t y) override;
  void handleTouch(int32_t x, int32_t y) override;
  void onButtonA() override;
  void onButtonB() override;

 private:
  enum class TouchAction : uint8_t {
    None,
    Toggle,
    Setup,
    Back,
  };

  void toggleWifi();
  void startSetup();
  void goBack();
  void drawStatus();
  void drawControls(TouchAction highlighted);
  void drawButton(int x, int y, int w, int h, const char* label, bool highlighted);
  TouchAction actionAt(int32_t x, int32_t y) const;
  String snapshot() const;

  SettingsStore& settings_;
  WifiService& wifi_;
  uint32_t lastDrawMs_ = 0;
  String lastSnapshot_;
  TouchAction highlightedAction_ = TouchAction::None;
};

}  // namespace iris
