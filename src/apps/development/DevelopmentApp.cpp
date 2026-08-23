#include "iris/apps/development/DevelopmentApp.h"

namespace iris {

bool DevelopmentApp::onButtonA() {
  screens_.onButtonA();
  return true;
}

bool DevelopmentApp::onButtonB() {
  screens_.onButtonB();
  return true;
}

bool DevelopmentApp::onTouch(int32_t x, int32_t y) {
  screens_.handleTouch(x, y);
  return true;
}

}  // namespace iris
