#pragma once

#include <Arduino.h>
#include <M5Unified.h>

#include "iris/services/SettingsStore.h"

namespace iris {

struct Theme {
  uint16_t background;
  uint16_t foreground;
  uint16_t muted;
  uint16_t accent;
  uint16_t panel;
  uint16_t selected;
  uint16_t button;
};

inline Theme currentTheme(const SettingsStore& settings) {
  switch (settings.watchBackground() % 5) {
    case 1:
      return {0x0010, TFT_WHITE, 0x867F, 0x867F, 0x1084, 0x29EF, 0x1084};
    case 2:
      return {0x0188, TFT_WHITE, 0xB7E0, 0xB7E0, 0x11C8, 0x2B4C, 0x11C8};
    case 3:
      return {0x2008, TFT_WHITE, 0xFBBF, 0xFBBF, 0x310C, 0x59D3, 0x310C};
    case 4:
      return {0x2104, TFT_WHITE, 0xD6BA, 0xD6BA, 0x39E7, 0x5AEB, 0x39E7};
    default:
      return {TFT_BLACK, TFT_WHITE, 0xBDF7, 0xBDF7, 0x2104, TFT_DARKGREY, 0x2104};
  }
}

inline const char* themeName(const SettingsStore& settings) {
  switch (settings.watchBackground() % 5) {
    case 1: return "Midnight";
    case 2: return "Forest";
    case 3: return "Plum";
    case 4: return "Steel";
    default: return "Black";
  }
}

}  // namespace iris
