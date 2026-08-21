#pragma once

#include <Arduino.h>
#include <M5Unified.h>

#include "iris/services/SettingsStore.h"

namespace iris {

struct Theme {
  const char* name;
  uint16_t background;
  uint16_t foreground;
  uint16_t muted;
  uint16_t accent;
  uint16_t panel;
  uint16_t selected;
  uint16_t button;
  uint16_t timePanel;
};

constexpr uint8_t kThemeCount = 5;

inline Theme currentTheme(const SettingsStore& settings) {
  switch (settings.themeId() % kThemeCount) {
    case 1:
      return {"Midnight", 0x0010, TFT_WHITE, 0x867F, 0x867F, 0x1084, 0x29EF, 0x1084, 0x0010};
    case 2:
      return {"Forest", 0x0188, TFT_WHITE, 0xB7E0, 0xB7E0, 0x11C8, 0x2B4C, 0x11C8, 0x0188};
    case 3:
      return {"Plum", 0x2008, TFT_WHITE, 0xFBBF, 0xFBBF, 0x310C, 0x59D3, 0x310C, 0x2008};
    case 4:
      return {"Steel", 0x2104, TFT_WHITE, 0xD6BA, 0xD6BA, 0x39E7, 0x5AEB, 0x39E7, 0x2104};
    default:
      return {"Black", TFT_BLACK, TFT_WHITE, 0xBDF7, 0xBDF7, 0x2104, TFT_DARKGREY, 0x2104, TFT_BLACK};
  }
}

inline const char* themeName(const SettingsStore& settings) {
  return currentTheme(settings).name;
}

}  // namespace iris
