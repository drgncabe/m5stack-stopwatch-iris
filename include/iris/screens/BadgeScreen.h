#pragma once

#include <Arduino.h>
#include <AnimatedGIF.h>

#include "iris/screens/Screen.h"
#include "iris/services/BadgeService.h"
#include "iris/services/SettingsStore.h"

namespace iris {

class BadgeScreen : public Screen {
 public:
  BadgeScreen(SettingsStore& settings, BadgeService& badge);

  void enter() override;
  void exit() override;
  void update(uint32_t nowMs) override;
  void draw() override;
  void handleTouch(int32_t x, int32_t y) override;
  void onButtonA() override;
  void onButtonB() override;

 private:
  void drawDefaultBadge();
  void drawImageBadge();
  void drawGifBadge();
  void closeGif();
  bool ensureGifOpen();
  bool playGifFrame(uint32_t nowMs);
  void drawInfoOverlay();
  void showInfo(uint32_t nowMs);
  float scaleFor(const BadgeMetadata& meta) const;
  static void drawGifLine(GIFDRAW* draw);

  SettingsStore& settings_;
  BadgeService& badge_;
  AnimatedGIF gif_;
  bool drawn_ = false;
  bool showInfo_ = false;
  bool gifOpen_ = false;
  uint32_t nextGifFrameMs_ = 0;
  int gifOffsetX_ = 0;
  int gifOffsetY_ = 0;
  int gifError_ = GIF_SUCCESS;
  uint32_t hideInfoAtMs_ = 0;
};

}  // namespace iris
