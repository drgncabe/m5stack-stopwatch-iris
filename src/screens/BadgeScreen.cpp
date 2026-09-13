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
constexpr int kDisplayW = 466;
constexpr int kDisplayH = 466;

fs::File sGifFile;

uint16_t dimColor(uint16_t color) {
  const uint8_t r = (color >> 11) & 0x1F;
  const uint8_t g = (color >> 5) & 0x3F;
  const uint8_t b = color & 0x1F;
  return static_cast<uint16_t>(((r / 2) << 11) | ((g / 2) << 5) | (b / 2));
}

void* gifOpenFile(const char* filename, int32_t* size) {
  sGifFile = SPIFFS.open(filename, FILE_READ);
  if (!sGifFile) return nullptr;
  *size = static_cast<int32_t>(sGifFile.size());
  return static_cast<void*>(&sGifFile);
}

void gifCloseFile(void* handle) {
  fs::File* file = static_cast<fs::File*>(handle);
  if (file) file->close();
}

int32_t gifReadFile(GIFFILE* file, uint8_t* buffer, int32_t length) {
  if (!file || !file->fHandle || !buffer || length <= 0) return 0;
  fs::File* fsFile = static_cast<fs::File*>(file->fHandle);
  int32_t bytesToRead = length;
  if ((file->iSize - file->iPos) < length) {
    bytesToRead = file->iSize - file->iPos;
  }
  if (bytesToRead <= 0) return 0;
  const int32_t bytesRead = static_cast<int32_t>(fsFile->read(buffer, bytesToRead));
  file->iPos = static_cast<int32_t>(fsFile->position());
  return bytesRead;
}

int32_t gifSeekFile(GIFFILE* file, int32_t position) {
  if (!file || !file->fHandle) return 0;
  fs::File* fsFile = static_cast<fs::File*>(file->fHandle);
  fsFile->seek(position);
  file->iPos = static_cast<int32_t>(fsFile->position());
  return file->iPos;
}
}  // namespace

BadgeScreen::BadgeScreen(SettingsStore& settings, BadgeService& badge)
    : settings_(settings), badge_(badge) {}

void BadgeScreen::enter() {
  closeGif();
  drawn_ = false;
  showInfo_ = false;
  draw();
}

void BadgeScreen::exit() {
  closeGif();
  showInfo_ = false;
}

void BadgeScreen::update(uint32_t nowMs) {
  if (gifOpen_ && nowMs >= nextGifFrameMs_) {
    playGifFrame(nowMs);
    if (showInfo_) drawInfoOverlay();
  }
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
      drawGifBadge();
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
  closeGif();
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

void BadgeScreen::drawGifBadge() {
  const Theme theme = currentTheme(settings_);
  const BadgeMetadata& meta = badge_.metadata();
  M5.Display.fillScreen(theme.background);
  if (ensureGifOpen() && playGifFrame(millis())) {
    return;
  }

  M5.Display.setTextDatum(middle_center);
  M5.Display.fillCircle(kCenter, kCenter, 128, theme.panel);
  M5.Display.drawCircle(kCenter, kCenter, 130, theme.accent);
  M5.Display.drawCircle(kCenter, kCenter, 154, dimColor(theme.accent));
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.setTextColor(theme.foreground, theme.panel);
  M5.Display.drawString("GIF", kCenter, 176);
  M5.Display.setFont(&fonts::FreeSans12pt7b);
  M5.Display.setTextColor(theme.accent, theme.panel);
  M5.Display.drawString("Unavailable", kCenter, 226);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(theme.muted, theme.background);
  M5.Display.drawString(meta.filename, kCenter, 332);
  M5.Display.drawString(String("GIF error ") + gifError_, kCenter, 362);
  M5.Display.drawString("A: Menu   B: Mode", kCenter, 424);
}

void BadgeScreen::closeGif() {
  if (!gifOpen_) return;
  gif_.close();
  gifOpen_ = false;
  nextGifFrameMs_ = 0;
}

bool BadgeScreen::ensureGifOpen() {
  if (gifOpen_) return true;
  const BadgeMetadata& meta = badge_.metadata();
  if (!badge_.hasAsset() || meta.type != BadgeAssetType::Gif) return false;

  gif_.begin(LITTLE_ENDIAN_PIXELS);
  const int result = gif_.open(meta.path.c_str(), gifOpenFile, gifCloseFile, gifReadFile,
                               gifSeekFile, BadgeScreen::drawGifLine);
  if (!result) {
    gifError_ = gif_.getLastError();
    closeGif();
    return false;
  }
  gifOpen_ = true;
  gifError_ = GIF_SUCCESS;
  gifScale_ = gifScaleFor(gif_.getCanvasWidth(), gif_.getCanvasHeight());
  gifOffsetX_ = (M5.Display.width() - (gif_.getCanvasWidth() * gifScale_)) / 2;
  gifOffsetY_ = (M5.Display.height() - (gif_.getCanvasHeight() * gifScale_)) / 2;
  nextGifFrameMs_ = 0;
  return true;
}

bool BadgeScreen::playGifFrame(uint32_t nowMs) {
  if (!ensureGifOpen()) return false;

  int delayMs = 0;
  int result = gif_.playFrame(false, &delayMs, this);
  if (!result) {
    gif_.reset();
    result = gif_.playFrame(false, &delayMs, this);
  }
  if (!result) {
    gifError_ = gif_.getLastError();
    closeGif();
    return false;
  }

  if (delayMs < 20) delayMs = 20;
  nextGifFrameMs_ = nowMs + static_cast<uint32_t>(delayMs);
  return true;
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

uint8_t BadgeScreen::gifScaleFor(int width, int height) const {
  if (width <= 0 || height <= 0) return 1;
  if (badge_.metadata().mode == BadgeDisplayMode::Center) return 1;

  const int fitScale = min(kDisplayW / width, kDisplayH / height);
  if (badge_.metadata().mode == BadgeDisplayMode::Fit) {
    return static_cast<uint8_t>(max(1, fitScale));
  }

  const int fillScale = max((kDisplayW + width - 1) / width, (kDisplayH + height - 1) / height);
  return static_cast<uint8_t>(max(1, fillScale));
}

void BadgeScreen::drawGifLine(GIFDRAW* draw) {
  if (!draw || !draw->pUser) return;
  BadgeScreen* screen = static_cast<BadgeScreen*>(draw->pUser);
  const int scale = max(1, static_cast<int>(screen->gifScale_));
  const int y = screen->gifOffsetY_ + ((draw->iY + draw->y) * scale);
  if (y >= kDisplayH || y + scale <= 0) return;

  int sourceX = 0;
  int x = screen->gifOffsetX_ + (draw->iX * scale);
  int width = draw->iWidth * scale;
  if (x < 0) {
    sourceX = (-x + scale - 1) / scale;
    width -= sourceX * scale;
    x += sourceX * scale;
    x = 0;
  }
  if (x + width > kDisplayW) width = kDisplayW - x;
  if (width <= 0) return;

  uint8_t* pixels = draw->pPixels + sourceX;
  uint16_t* palette = draw->pPalette;
  uint16_t line[kDisplayW];
  const int sourceWidth = min(draw->iWidth - sourceX, (width + scale - 1) / scale);

  if (!draw->ucHasTransparency) {
    int out = 0;
    for (int i = 0; i < sourceWidth && out < width; ++i) {
      const uint16_t color = palette[pixels[i]];
      for (int repeat = 0; repeat < scale && out < width; ++repeat) {
        line[out++] = color;
      }
    }
    for (int repeatY = 0; repeatY < scale; ++repeatY) {
      const int drawY = y + repeatY;
      if (drawY >= 0 && drawY < kDisplayH) M5.Display.pushImage(x, drawY, width, 1, line);
    }
    return;
  }

  int runStart = -1;
  int out = 0;
  for (int i = 0; i < sourceWidth && out < width; ++i) {
    if (pixels[i] == draw->ucTransparent) {
      if (runStart >= 0) {
        for (int repeatY = 0; repeatY < scale; ++repeatY) {
          const int drawY = y + repeatY;
          if (drawY >= 0 && drawY < kDisplayH) {
            M5.Display.pushImage(x + runStart, drawY, out - runStart, 1, line + runStart);
          }
        }
        runStart = -1;
      }
      out += scale;
      continue;
    }
    const uint16_t color = palette[pixels[i]];
    for (int repeat = 0; repeat < scale && out < width; ++repeat) {
      if (runStart < 0) runStart = out;
      line[out++] = color;
    }
  }
  if (runStart >= 0) {
    for (int repeatY = 0; repeatY < scale; ++repeatY) {
      const int drawY = y + repeatY;
      if (drawY >= 0 && drawY < kDisplayH) {
        M5.Display.pushImage(x + runStart, drawY, out - runStart, 1, line + runStart);
      }
    }
  }
}

}  // namespace iris
