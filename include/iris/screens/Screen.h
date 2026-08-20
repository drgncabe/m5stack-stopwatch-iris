#pragma once

#include <Arduino.h>

namespace iris {

class ScreenManager;

enum class ScreenId : uint8_t {
  Watch = 0,
  MainMenu,
  Settings,
  Volume,
  Wifi,
  DeviceInfo,
  Count
};

class Screen {
 public:
  virtual ~Screen() = default;

  void attach(ScreenManager* manager) { manager_ = manager; }

  virtual void enter() = 0;
  virtual void update(uint32_t nowMs) = 0;
  virtual void draw() = 0;
  virtual void handleTouch(int32_t x, int32_t y) = 0;
  virtual void onButtonA() {}
  virtual void onButtonB() {}

 protected:
  ScreenManager* manager_ = nullptr;
};

}  // namespace iris
