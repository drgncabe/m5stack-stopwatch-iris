#include "iris/apps/media/BadgeApp.h"

namespace iris {

void BadgeApp::onStart() {
  applyWakeLock();
}

void BadgeApp::onResume() {
  applyWakeLock();
}

void BadgeApp::onPause() {
  releaseWakeLock();
}

void BadgeApp::onStop() {
  releaseWakeLock();
}

void BadgeApp::refreshWakeLock() {
  if (badge_.keepAwake()) {
    applyWakeLock();
  } else {
    releaseWakeLock();
  }
}

void BadgeApp::applyWakeLock() {
  if (displayRequested_ || !badge_.keepAwake()) return;
  power_.requestDisplay();
  displayRequested_ = true;
}

void BadgeApp::releaseWakeLock() {
  if (!displayRequested_) return;
  power_.releaseDisplay();
  displayRequested_ = false;
}

}  // namespace iris
