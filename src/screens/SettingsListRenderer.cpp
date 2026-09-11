#include "iris/screens/SettingsListRenderer.h"

#include <M5Unified.h>

namespace iris {

void SettingsListRenderer::drawTitle(const Theme& theme, const char* title, int y) {
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.drawString(title, M5.Display.width() / 2, y);
}

void SettingsListRenderer::drawFooter(const Theme& theme, const String& line1, const String& line2,
                                      int y1, int y2) {
  M5.Display.fillRect(18, y1 - 12, 430, max(18, y2 - y1 + 20), theme.background);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(theme.muted, theme.background);
  if (!line1.isEmpty()) {
    M5.Display.drawString(fitText(line1, 390), M5.Display.width() / 2, y1);
  }
  if (!line2.isEmpty()) {
    M5.Display.drawString(fitText(line2, 390), M5.Display.width() / 2, y2);
  }
}

void SettingsListRenderer::drawRow(const Theme& theme, const SettingsListLayout& layout,
                                   size_t index, const String& label, const String& value,
                                   bool selected) {
  const int y = layout.rowStartY + static_cast<int>(index) * layout.rowHeight;
  const uint16_t fill = selected ? theme.selected : theme.background;
  const uint16_t border = selected ? theme.foreground : theme.panel;

  M5.Display.fillRoundRect(layout.rowLeft, y, layout.rowWidth, layout.rowRectHeight,
                           layout.rowRadius, fill);
  M5.Display.drawRoundRect(layout.rowLeft, y, layout.rowWidth, layout.rowRectHeight,
                           layout.rowRadius, border);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextDatum(middle_left);
  M5.Display.setTextColor(theme.foreground, fill);
  M5.Display.drawString(fitText(label, layout.labelWidth), layout.rowLeft + layout.labelPadding,
                        y + (layout.rowRectHeight / 2));
  drawRowValue(theme, layout, index, value, selected);
}

void SettingsListRenderer::drawRowValue(const Theme& theme, const SettingsListLayout& layout,
                                        size_t index, const String& value, bool selected) {
  const int y = layout.rowStartY + static_cast<int>(index) * layout.rowHeight;
  const uint16_t fill = selected ? theme.selected : theme.background;
  const int valueRight = layout.rowLeft + layout.rowWidth - layout.valuePadding;
  const int valueLeft = valueRight - layout.valueWidth;

  M5.Display.fillRect(valueLeft - 4, y + 2, layout.valueWidth + 8, layout.rowRectHeight - 4, fill);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextDatum(middle_right);
  M5.Display.setTextColor(theme.muted, fill);
  M5.Display.drawString(fitText(value, layout.valueWidth), valueRight,
                        y + (layout.rowRectHeight / 2));
}

void SettingsListRenderer::drawButton(const Theme& theme, int x, int y, int w, int h,
                                      const char* label, bool highlighted, int radius) {
  const uint16_t fill = highlighted ? theme.selected : theme.button;
  const uint16_t border = highlighted ? theme.foreground : theme.panel;
  M5.Display.fillRoundRect(x, y, w, h, radius, fill);
  M5.Display.drawRoundRect(x, y, w, h, radius, border);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(theme.foreground, fill);
  M5.Display.drawString(label, x + (w / 2), y + (h / 2));
}

int SettingsListRenderer::rowAt(const SettingsListLayout& layout, size_t itemCount,
                                int32_t x, int32_t y) {
  if (x < layout.rowLeft || x > layout.rowLeft + layout.rowWidth) return -1;
  for (size_t i = 0; i < itemCount; ++i) {
    const int rowY = layout.rowStartY + static_cast<int>(i) * layout.rowHeight;
    if (y >= rowY - 3 && y <= rowY + layout.rowRectHeight + 3) return static_cast<int>(i);
  }
  return -1;
}

String SettingsListRenderer::fitText(const String& text, int maxWidth) {
  if (maxWidth <= 0 || M5.Display.textWidth(text) <= maxWidth) return text;

  String fitted(text);
  const String ellipsis("...");
  while (fitted.length() > 0 && M5.Display.textWidth(fitted + ellipsis) > maxWidth) {
    fitted.remove(fitted.length() - 1);
  }
  return fitted.length() == 0 ? ellipsis : fitted + ellipsis;
}

}  // namespace iris
