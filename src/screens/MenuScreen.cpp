#include "iris/screens/MenuScreen.h"

#include <math.h>

#include <M5Unified.h>
#include "iris/Theme.h"
#include "iris/screens/ScreenManager.h"

namespace iris {

namespace {
constexpr int kTitleY = 42;
constexpr int kListCenterY = 230;
constexpr int kRowHeight = 58;
constexpr int kFooterY = 414;
constexpr int kVisiblePaddingRows = 2;
constexpr int kTouchScrollStepPx = 72;
constexpr int kDragDeadzonePx = 3;
constexpr int kMaxDragDeltaPx = 24;
constexpr int kScreenSafeMarginPx = 18;
constexpr int kTextSafePaddingPx = 18;
constexpr uint32_t kHapticPulseMs = 14;
constexpr uint32_t kScrollIndicatorHoldMs = 750;

uint16_t blend565(uint16_t fg, uint16_t bg, uint8_t amount) {
  const uint8_t fr = ((fg >> 11) & 0x1F) << 3;
  const uint8_t fgG = ((fg >> 5) & 0x3F) << 2;
  const uint8_t fb = (fg & 0x1F) << 3;
  const uint8_t br = ((bg >> 11) & 0x1F) << 3;
  const uint8_t bgG = ((bg >> 5) & 0x3F) << 2;
  const uint8_t bb = (bg & 0x1F) << 3;
  const uint8_t r = br + (((fr - br) * amount) / 255);
  const uint8_t g = bgG + (((fgG - bgG) * amount) / 255);
  const uint8_t b = bb + (((fb - bb) * amount) / 255);
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

int circularSafeWidthAtY(int y, int margin) {
  const int width = M5.Display.width();
  const int height = M5.Display.height();
  const int centerY = height / 2;
  const int radius = (min(width, height) / 2) - margin;
  const int dy = abs(y - centerY);
  if (dy >= radius) return 0;

  const float halfWidth = sqrtf(static_cast<float>((radius * radius) - (dy * dy)));
  return max(0, static_cast<int>(halfWidth * 2.0f) - (margin * 2));
}

String fitTextToWidth(const char* text, int maxWidth) {
  String fitted(text);
  if (maxWidth <= 0 || M5.Display.textWidth(fitted) <= maxWidth) return fitted;

  const String ellipsis("...");
  while (fitted.length() > 0 &&
         M5.Display.textWidth(fitted + ellipsis) > maxWidth) {
    fitted.remove(fitted.length() - 1);
  }

  if (fitted.length() == 0) return ellipsis;
  return fitted + ellipsis;
}
}  // namespace

MenuScreen::MenuScreen(const char* title, const MenuItem* items, size_t itemCount,
                       SettingsStore& settings, bool wrapEnabled)
    : title_(title),
      items_(items),
      itemCount_(itemCount),
      settings_(settings),
      wrapEnabled_(wrapEnabled) {}

void MenuScreen::enter() {
  selected_ = 0;
  touchGestureActive_ = false;
  draggedSinceTouch_ = false;
  lastTouchY_ = 0;
  scrollRemainder_ = 0;
  scrollIndicatorDrawn_ = false;
  lastScrollActivityMs_ = 0;
  stopSelectionHaptic();
}

void MenuScreen::update(uint32_t nowMs) {
  if (hapticActive_ && nowMs >= hapticOffMs_) {
    stopSelectionHaptic();
  }
  if (scrollIndicatorDrawn_ && nowMs - lastScrollActivityMs_ > kScrollIndicatorHoldMs) {
    draw();
  }
}

void MenuScreen::draw() {
  const Theme theme = currentTheme(settings_);
  M5.Display.fillScreen(theme.background);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.drawString(fitTextToWidth(title_, circularSafeWidthAtY(kTitleY, 26)),
                        M5.Display.width() / 2, kTitleY);

  for (int relative = -kVisiblePaddingRows; relative <= kVisiblePaddingRows; ++relative) {
    const int row = static_cast<int>(selected_) + relative;
    if (row < 0 || row >= static_cast<int>(itemCount_)) continue;
    drawRow(static_cast<size_t>(row), relative);
  }
  drawScrollBar(millis());

  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(theme.muted, theme.background);
  M5.Display.drawString(fitTextToWidth("A: Next   B: Select",
                                       circularSafeWidthAtY(kFooterY, 26)),
                        M5.Display.width() / 2, kFooterY);
}

void MenuScreen::previewTouch(int32_t x, int32_t y) {
  (void)x;
  if (itemCount_ == 0) return;

  if (!touchGestureActive_) {
    touchGestureActive_ = true;
    draggedSinceTouch_ = false;
    lastTouchY_ = y;
    scrollRemainder_ = 0;
    return;
  }

  int32_t deltaY = y - lastTouchY_;
  lastTouchY_ = y;
  if (abs(deltaY) < kDragDeadzonePx) return;
  deltaY = constrain(deltaY, -kMaxDragDeltaPx, kMaxDragDeltaPx);
  scrollRemainder_ += deltaY;

  while (scrollRemainder_ <= -kTouchScrollStepPx) {
    scrollSelection(1);
    draggedSinceTouch_ = true;
    scrollRemainder_ += kTouchScrollStepPx;
  }

  while (scrollRemainder_ >= kTouchScrollStepPx) {
    scrollSelection(-1);
    draggedSinceTouch_ = true;
    scrollRemainder_ -= kTouchScrollStepPx;
  }
}

void MenuScreen::handleTouch(int32_t x, int32_t y) {
  (void)x;
  (void)y;
  const bool touchIsStillPressed = M5.Touch.getDetail().isPressed();
  touchGestureActive_ = false;
  scrollRemainder_ = 0;

  if (!touchIsStillPressed || draggedSinceTouch_) {
    draggedSinceTouch_ = false;
    return;
  }

  activateSelected();
}

void MenuScreen::onButtonA() {
  if (itemCount_ == 0) return;
  if (wrapEnabled_) {
    selectRow((selected_ + 1) % itemCount_);
  } else {
    scrollSelection(1);
  }
}

void MenuScreen::onButtonB() {
  activateSelected();
}

void MenuScreen::setWrapEnabled(bool enabled) {
  wrapEnabled_ = enabled;
}

void MenuScreen::activateSelected() {
  if (!manager_ || selected_ >= itemCount_) return;
  manager_->show(items_[selected_].target);
}

void MenuScreen::selectRow(size_t row) {
  if (row >= itemCount_ || row == selected_) return;
  selected_ = row;
  lastScrollActivityMs_ = millis();
  draw();
  startSelectionHaptic();
}

void MenuScreen::scrollSelection(int direction) {
  if (itemCount_ == 0) return;
  const int next = constrain(static_cast<int>(selected_) + direction, 0,
                             static_cast<int>(itemCount_) - 1);
  selectRow(static_cast<size_t>(next));
}

void MenuScreen::drawRow(size_t index, int relativePosition) {
  if (index >= itemCount_) return;
  const Theme theme = currentTheme(settings_);
  if (abs(relativePosition) > kVisiblePaddingRows) return;

  const bool selected = relativePosition == 0;
  const int distance = abs(relativePosition);
  const int y = kListCenterY + relativePosition * kRowHeight;
  const int displayCenterX = M5.Display.width() / 2;
  const int inset = distance == 0 ? 40 : (distance == 1 ? 66 : 108);
  const int maxSafeWidth = circularSafeWidthAtY(y, kScreenSafeMarginPx);
  const int rowHeight = distance == 0 ? 52 : (distance == 1 ? 42 : 34);
  const int desiredRowW = M5.Display.width() - (inset * 2);
  const int rowW = min(desiredRowW, maxSafeWidth);
  const int rowX = displayCenterX - (rowW / 2);
  const int textMaxWidth = max(0, rowW - (kTextSafePaddingPx * 2));
  const uint8_t brightness = distance == 0 ? 255 : (distance == 1 ? 175 : 105);
  const uint16_t text = blend565(theme.foreground, theme.background, brightness);

  if (selected) {
    M5.Display.fillRoundRect(rowX, y - (rowHeight / 2), rowW, rowHeight, 20, theme.selected);
    M5.Display.drawRoundRect(rowX, y - (rowHeight / 2), rowW, rowHeight, 20, theme.accent);
  }
  M5.Display.setTextDatum(middle_center);
  if (selected) {
    M5.Display.setFont(&fonts::FreeSansBold12pt7b);
  } else if (distance == 1) {
    M5.Display.setFont(&fonts::FreeSans12pt7b);
  } else {
    M5.Display.setFont(&fonts::FreeSans9pt7b);
  }
  M5.Display.setTextColor(text, selected ? theme.selected : theme.background);
  M5.Display.drawString(fitTextToWidth(items_[index].label, textMaxWidth), displayCenterX, y);
}

void MenuScreen::drawScrollBar(uint32_t nowMs) {
  if (itemCount_ <= static_cast<size_t>((kVisiblePaddingRows * 2) + 1)) return;
  if (lastScrollActivityMs_ == 0 || nowMs - lastScrollActivityMs_ > kScrollIndicatorHoldMs) {
    scrollIndicatorDrawn_ = false;
    return;
  }
  const Theme theme = currentTheme(settings_);
  constexpr int trackX = 410;
  constexpr int trackY = 112;
  constexpr int trackH = 224;
  constexpr int trackW = 5;
  constexpr int thumbMinH = 34;
  const int thumbH = max(thumbMinH, trackH / static_cast<int>(itemCount_));
  const int travel = trackH - thumbH;
  const int thumbY = trackY + (travel * static_cast<int>(selected_)) /
                                  static_cast<int>(itemCount_ - 1);
  scrollIndicatorDrawn_ = true;
  M5.Display.fillRoundRect(trackX, trackY, trackW, trackH, 3, theme.panel);
  M5.Display.fillRoundRect(trackX - 1, thumbY, trackW + 2, thumbH, 4, theme.accent);
}

void MenuScreen::startSelectionHaptic() {
  M5.Power.setVibration(82);
  hapticActive_ = true;
  hapticOffMs_ = millis() + kHapticPulseMs;
}

void MenuScreen::stopSelectionHaptic() {
  if (!hapticActive_) return;
  M5.Power.setVibration(0);
  hapticActive_ = false;
}

}  // namespace iris
