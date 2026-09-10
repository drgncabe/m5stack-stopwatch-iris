#pragma once

#include <Arduino.h>

#include "iris/Theme.h"

namespace iris {

struct SettingsListLayout {
  constexpr SettingsListLayout() = default;
  constexpr SettingsListLayout(int rowHeightValue, int rowStartYValue, int rowLeftValue,
                               int rowWidthValue, int rowRectHeightValue, int rowRadiusValue,
                               int labelPaddingValue, int valuePaddingValue,
                               int labelWidthValue, int valueWidthValue, int footerYValue)
      : rowHeight(rowHeightValue),
        rowStartY(rowStartYValue),
        rowLeft(rowLeftValue),
        rowWidth(rowWidthValue),
        rowRectHeight(rowRectHeightValue),
        rowRadius(rowRadiusValue),
        labelPadding(labelPaddingValue),
        valuePadding(valuePaddingValue),
        labelWidth(labelWidthValue),
        valueWidth(valueWidthValue),
        footerY(footerYValue) {}

  int rowHeight = 32;
  int rowStartY = 70;
  int rowLeft = 42;
  int rowWidth = 382;
  int rowRectHeight = 27;
  int rowRadius = 10;
  int labelPadding = 16;
  int valuePadding = 16;
  int labelWidth = 178;
  int valueWidth = 184;
  int footerY = 438;
};

class SettingsListRenderer {
 public:
  static constexpr SettingsListLayout defaultLayout() { return SettingsListLayout{}; }

  static void drawTitle(const Theme& theme, const char* title, int y = 50);
  static void drawFooter(const Theme& theme, const String& line1, const String& line2,
                         int y1 = 428, int y2 = 448);
  static void drawRow(const Theme& theme, const SettingsListLayout& layout, size_t index,
                      const String& label, const String& value, bool selected);
  static void drawRowValue(const Theme& theme, const SettingsListLayout& layout, size_t index,
                           const String& value, bool selected);
  static void drawButton(const Theme& theme, int x, int y, int w, int h, const char* label,
                         bool highlighted = false, int radius = 16);
  static int rowAt(const SettingsListLayout& layout, size_t itemCount, int32_t x, int32_t y);
  static String fitText(const String& text, int maxWidth);
};

}  // namespace iris
