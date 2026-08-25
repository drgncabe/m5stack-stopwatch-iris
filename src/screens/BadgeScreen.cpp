#include "iris/screens/BadgeScreen.h"

#include <M5Unified.h>
#include <SPIFFS.h>
#include <math.h>

#include "iris/Theme.h"
#include "iris/screens/ScreenManager.h"

namespace iris {

namespace {
constexpr int kCenter = 233;
constexpr uint32_t kInfoMs = 3500;

uint16_t dimColor(uint16_t color) {
  const uint8_t r = (color >> 11) & 0x1F;
  const uint8_t g = (color >> 5) & 0x3F;
  const uint8_t b = color & 0x1F;
  return static_cast<uint16_t>(((r / 2) << 11) | ((g / 2) << 5) | (b / 2));
}
}  // namespace

BadgeScreen::BadgeScreen(SettingsStore& settings, BadgeService& badge)
    : settings_(settings), badge_(badge) {}

void BadgeScreen::enter() {
  drawn_ = false;
  showInfo_ = false;
  draw();
}

void BadgeScreen::update(uint32_t nowMs) {
  if (showInfo_ && nowMs >= hideInfoAtMs_) {
    showInfo_ = false;
    drawn_ = false;
    draw();
  }
}

void BadgeScreen::draw() {
  if (!drawn_) {
    if (!badge_.hasAsset()) {
      drawDefaultBadge();
    } else if (badge_.isStaticRenderable()) {
      drawImageBadge();
    } else {
      drawGifPlaceholder();
    }
    drawn_ = true;
  }

  if (showInfo_) {
    drawInfoOverlay();
  }
}

void BadgeScreen::handleTouch(int32_t, int32_t y) {
  if (y > 360 && manager_) {
    manager_->show(ScreenId::MainMenu);
    return;
  }
  showInfo(millis());
}

void BadgeScreen::onButtonA() {
  if (manager_) manager_->show(ScreenId::MainMenu);
}

void BadgeScreen::onButtonB() {
  badge_.nextMode();
  drawn_ = false;
  showInfo(millis());
}

void BadgeScreen::drawDefaultBadge() {
  const Theme theme = currentTheme(settings_);
  M5.Display.fillScreen(theme.background);
  M5.Display.setTextDatum(middle_center);

  for (int r = 196; r >= 72; r -= 24) {
    M5.Display.drawCircle(kCenter, kCenter, r, r % 48 == 0 ? theme.panel : dimColor(theme.panel));
  }
  M5.Display.fillCircle(kCenter, kCenter, 118, theme.panel);
  M5.Display.drawCircle(kCenter, kCenter, 120, theme.accent);
  M5.Display.drawCircle(kCenter, kCenter, 132, dimColor(theme.accent));

  M5.Display.setFont(&fonts::FreeSansBold24pt7b);
  M5.Display.setTextColor(theme.foreground, theme.panel);
  M5.Display.drawString("IRIS", kCenter, 196);
  M5.Display.setFont(&fonts::FreeSans12pt7b);
  M5.Display.setTextColor(theme.accent, theme.panel);
  M5.Display.drawString("BADGE", kCenter, 250);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(theme.muted, theme.background);
  M5.Display.drawString("Upload media from web", kCenter, 386);
  M5.Display.drawString("A: Menu   B: Mode", kCenter, 424);
}

void BadgeScreen::drawImageBadge() {
  const BadgeMetadata& meta = badge_.metadata();
  const Theme theme = currentTheme(settings_);
  M5.Display.fillScreen(theme.background);

  const float scale = scaleFor(meta);
  const int32_t scaledW = static_cast<int32_t>(meta.width * scale);
  const int32_t scaledH = static_cast<int32_t>(meta.height * scale);
  const int32_t x = (M5.Display.width() - scaledW) / 2;
  const int32_t y = (M5.Display.height() - scaledH) / 2;
  bool ok = false;
  fs::File file = SPIFFS.open(meta.path, FILE_READ);
  if (!file) {
    ok = false;
  } else if (meta.type == BadgeAssetType::Jpeg) {
    ok = M5.Display.drawJpg(&file, x, y, 466, 466, 0, 0, scale);
  } else if (meta.type == BadgeAssetType::Png) {
    ok = M5.Display.drawPng(&file, x, y, 466, 466, 0, 0, scale);
  }
  if (file) file.close();

  if (!ok) {
    M5.Display.fillScreen(theme.background);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setFont(&fonts::FreeSansBold12pt7b);
    M5.Display.setTextColor(theme.foreground, theme.background);
    M5.Display.drawString("Badge unavailable", kCenter, 190);
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.setTextColor(theme.muted, theme.background);
    M5.Display.drawString(meta.filename, kCenter, 232);
    M5.Display.drawString("A: Menu   B: Mode", kCenter, 424);
  }
}

void BadgeScreen::drawGifPlaceholder() {
  const Theme theme = currentTheme(settings_);
  const BadgeMetadata& meta = badge_.metadata();
  M5.Display.fillScreen(theme.background);
  M5.Display.setTextDatum(middle_center);
  M5.Display.fillCircle(kCenter, kCenter, 128, theme.panel);
  M5.Display.drawCircle(kCenter, kCenter, 130, theme.accent);
  M5.Display.drawCircle(kCenter, kCenter, 154, dimColor(theme.accent));
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.setTextColor(theme.foreground, theme.panel);
  M5.Display.drawString("GIF", kCenter, 176);
  M5.Display.setFont(&fonts::FreeSans12pt7b);
  M5.Display.setTextColor(theme.accent, theme.panel);
  M5.Display.drawString("Stored", kCenter, 226);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(theme.muted, theme.background);
  M5.Display.drawString(meta.filename, kCenter, 332);
  M5.Display.drawString("Animation decoder pending", kCenter, 362);
  M5.Display.drawString("A: Menu   B: Mode", kCenter, 424);
}

void BadgeScreen::drawInfoOverlay() {
  const Theme theme = currentTheme(settings_);
  const BadgeMetadata& meta = badge_.metadata();
  M5.Display.fillRoundRect(56, 322, 354, 88, 14, theme.panel);
  M5.Display.drawRoundRect(56, 322, 354, 88, 14, theme.accent);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(theme.foreground, theme.panel);
  M5.Display.drawString(badge_.hasAsset() ? meta.filename : String("Default Iris badge"), kCenter, 346);
  M5.Display.setTextColor(theme.muted, theme.panel);
  String detail = badge_.typeName() + " / " + badge_.modeName();
  if (meta.width > 0 && meta.height > 0) {
    detail += " / ";
    detail += String(meta.width);
    detail += "x";
    detail += String(meta.height);
  }
  M5.Display.drawString(detail, kCenter, 374);
  M5.Display.drawString("A: Menu   B: Mode", kCenter, 398);
}

void BadgeScreen::showInfo(uint32_t nowMs) {
  showInfo_ = true;
  hideInfoAtMs_ = nowMs + kInfoMs;
  draw();
}

float BadgeScreen::scaleFor(const BadgeMetadata& meta) const {
  if (meta.width == 0 || meta.height == 0) return 1.0f;
  const float sx = static_cast<float>(M5.Display.width()) / static_cast<float>(meta.width);
  const float sy = static_cast<float>(M5.Display.height()) / static_cast<float>(meta.height);
  if (meta.mode == BadgeDisplayMode::Fill) return sx > sy ? sx : sy;
  if (meta.mode == BadgeDisplayMode::Center && meta.width <= M5.Display.width() &&
      meta.height <= M5.Display.height()) {
    return 1.0f;
  }
  return sx < sy ? sx : sy;
}

}  // namespace iris
