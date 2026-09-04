#pragma once

#include <M5Unified.h>

#include "iris/screens/Screen.h"
#include "iris/services/RagnarLinkService.h"
#include "iris/services/SettingsStore.h"

namespace iris {

class RagnarLinkScreen : public Screen {
 public:
  RagnarLinkScreen(SettingsStore& settings, RagnarLinkService& ragnar);

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
    Back,
    ChannelDown,
    ChannelUp,
  };

  void drawButton(int x, int y, int w, int h, const char* label, bool highlighted);
  void drawMetric(int x, int y, const char* label, const String& value);
  void changeChannel(int delta);
  TouchAction actionAt(int32_t x, int32_t y) const;
  String stateSnapshot() const;
  String formatUptime(uint32_t seconds) const;

  SettingsStore& settings_;
  RagnarLinkService& ragnar_;
  M5Canvas canvas_;
  bool canvasReady_ = false;
  uint32_t lastDrawMs_ = 0;
  String lastSnapshot_;
  TouchAction highlightedAction_ = TouchAction::None;
};

}  // namespace iris
