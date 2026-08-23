#include "iris/apps/watch/WatchApp.h"

namespace iris {

bool WatchApp::onButtonA() {
  screen_.onButtonA();
  return true;
}

bool WatchApp::onButtonB() {
  screen_.onButtonB();
  return true;
}

bool WatchApp::onTouch(int32_t x, int32_t y) {
  screen_.handleTouch(x, y);
  return true;
}

}  // namespace iris
