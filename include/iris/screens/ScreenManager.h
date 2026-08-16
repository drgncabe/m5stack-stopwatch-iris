#pragma once

#include <array>
#include "iris/screens/Screen.h"

namespace iris {

class ScreenManager {
 public:
  ScreenManager();

  void registerScreen(ScreenId id, Screen* screen);
  void show(ScreenId id);
  void update(uint32_t nowMs);
  void handleTouch(int32_t x, int32_t y);
  void onButtonA();
  void onButtonB();

  ScreenId currentId() const { return currentId_; }

 private:
  std::array<Screen*, static_cast<size_t>(ScreenId::Count)> screens_{};
  Screen* current_ = nullptr;
  ScreenId currentId_ = ScreenId::Watch;
};

}  // namespace iris
