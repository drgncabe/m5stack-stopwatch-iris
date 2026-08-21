#pragma once

#include <Arduino.h>
#include <M5Unified.h>

#include "iris/services/SettingsStore.h"

namespace iris {

enum class WatchLayout : uint8_t {
  Classic,
  Compact,
  Focus,
};

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
  uint16_t complicationPanel;
  WatchLayout layout;
  int16_t brandY;
  int16_t timeY;
  int16_t dateY;
  int16_t complicationY;
  int16_t wifiY;
  bool showBrand;
  bool useTimePanel;
};

constexpr uint8_t kThemeCount = 5;

inline Theme currentTheme(const SettingsStore& settings) {
  switch (settings.themeId() % kThemeCount) {
    case 1:
      return {"Midnight", 0x0010, TFT_WHITE, 0x867F, 0x867F, 0x1084, 0x29EF, 0x1084,
              0x0010, 0x1084, WatchLayout::Focus, 0, 190, 270, 320, 360, false, false};
    case 2:
      return {"Forest", 0x0188, TFT_WHITE, 0xB7E0, 0xB7E0, 0x11C8, 0x2B4C, 0x11C8,
              0x0188, 0x11C8, WatchLayout::Compact, 58, 186, 258, 306, 354, true, false};
    case 3:
      return {"Plum", 0x2008, TFT_WHITE, 0xFBBF, 0xFBBF, 0x310C, 0x59D3, 0x310C,
              0x310C, 0x310C, WatchLayout::Classic, 72, 198, 272, 320, 360, true, true};
    case 4:
      return {"Steel", 0x2104, TFT_WHITE, 0xD6BA, 0xD6BA, 0x39E7, 0x5AEB, 0x39E7,
              0x2104, 0x39E7, WatchLayout::Compact, 58, 188, 258, 310, 356, true, false};
    default:
      return {"Black", TFT_BLACK, TFT_WHITE, 0xBDF7, 0xBDF7, 0x2104, TFT_DARKGREY, 0x2104,
              TFT_BLACK, 0x2104, WatchLayout::Classic, 72, 198, 272, 320, 360, true, true};
  }
}

inline const char* themeName(const SettingsStore& settings) {
  return currentTheme(settings).name;
}

inline const char* watchLayoutName(WatchLayout layout) {
  switch (layout) {
    case WatchLayout::Compact: return "Compact";
    case WatchLayout::Focus: return "Focus";
    default: return "Classic";
  }
}

}  // namespace iris
