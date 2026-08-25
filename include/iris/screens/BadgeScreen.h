#pragma once

#include <Arduino.h>

#include "iris/screens/Screen.h"
#include "iris/services/BadgeService.h"
#include "iris/services/SettingsStore.h"

namespace iris {

class BadgeScreen : public Screen {
 public:
  BadgeScreen(SettingsStore& settings, BadgeService& badge);

  void enter() override;
  void update(uint32_t nowMs) override;
  void draw() override;
  void handleTouch(int32_t x, int32_t y) override;
  void onButtonA() override;
  void onButtonB() override;

 private:
  void drawDefaultBadge();
  void drawImageBadge();
  void drawGifPlaceholder();
  void drawInfoOverlay();
  void showInfo(uint32_t nowMs);
  float scaleFor(const BadgeMetadata& meta) const;

  SettingsStore& settings_;
  BadgeService& badge_;
  bool drawn_ = false;
  bool showInfo_ = false;
  uint32_t hideInfoAtMs_ = 0;
};

}  // namespace iris
