#include "iris/apps/settings/SettingsApp.h"

namespace iris {

bool SettingsApp::onButtonA() {
  screens_.onButtonA();
  return true;
}

bool SettingsApp::onButtonB() {
  screens_.onButtonB();
  return true;
}

bool SettingsApp::onTouch(int32_t x, int32_t y) {
  screens_.handleTouch(x, y);
  return true;
}

}  // namespace iris
