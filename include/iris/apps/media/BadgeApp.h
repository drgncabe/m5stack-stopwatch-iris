#pragma once

#include "iris/core/AppManager.h"
#include "iris/services/BadgeService.h"
#include "iris/services/PowerManager.h"

namespace iris {

class BadgeApp : public IrisApplication {
 public:
  BadgeApp(BadgeService& badge, PowerManager& power) : badge_(badge), power_(power) {}

  const char* id() const override { return "media.badge"; }
  const char* name() const override { return "Badge"; }
  void onStart() override;
  void onResume() override;
  void onPause() override;
  void onStop() override;
  void refreshWakeLock();

 private:
  void applyWakeLock();
  void releaseWakeLock();

  BadgeService& badge_;
  PowerManager& power_;
  bool displayRequested_ = false;
};

}  // namespace iris
