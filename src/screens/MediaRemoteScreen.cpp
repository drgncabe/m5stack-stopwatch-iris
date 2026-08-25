#include "iris/screens/MediaRemoteScreen.h"

#include <M5Unified.h>

#include "iris/Theme.h"
#include "iris/screens/ScreenManager.h"

namespace iris {

namespace {
constexpr int kCenter = 233;
constexpr uint32_t kFeedbackMs = 260;
constexpr int kBackX = 28;
constexpr int kBackY = 24;
constexpr int kBackW = 92;
constexpr int kBackH = 44;

uint16_t dimColor(uint16_t color) {
  const uint8_t r = (color >> 11) & 0x1F;
  const uint8_t g = (color >> 5) & 0x3F;
  const uint8_t b = color & 0x1F;
  return static_cast<uint16_t>(((r / 2) << 11) | ((g / 2) << 5) | (b / 2));
}
}  // namespace

MediaRemoteScreen::MediaRemoteScreen(SettingsStore& settings, BluetoothService& bluetooth)
    : settings_(settings), bluetooth_(bluetooth) {}

void MediaRemoteScreen::enter() {
  view_ = bluetooth_.bondedDeviceCount() > 0 || bluetooth_.connected()
              ? View::Remote
              : View::BleInfo;
  preview_ = Target::None;
  lastSent_ = Target::None;
  feedbackUntilMs_ = 0;
  draw();
}

void MediaRemoteScreen::update(uint32_t nowMs) {
  updateHaptic(nowMs);
  if (feedbackUntilMs_ != 0 && static_cast<int32_t>(nowMs - feedbackUntilMs_) >= 0) {
    feedbackUntilMs_ = 0;
    lastSent_ = Target::None;
    draw();
  }
}

void MediaRemoteScreen::draw() {
  if (view_ == View::BleInfo) {
    drawBleInfo();
  } else {
    drawRemote();
  }
}

void MediaRemoteScreen::previewTouch(int32_t x, int32_t y) {
  Target next = targetAt(x, y);
  if (next == preview_) return;
  preview_ = next;
  if (view_ == View::Remote) drawRemote();
}

void MediaRemoteScreen::handleTouch(int32_t x, int32_t y) {
  Target target = targetAt(x, y);
  preview_ = Target::None;
  if (view_ == View::BleInfo) {
    if (target == Target::Pair) {
      bluetooth_.setEnabled(true);
      bluetooth_.startAdvertising();
      view_ = View::Remote;
    } else if (target == Target::Menu && manager_) {
      manager_->show(ScreenId::MainMenu);
      return;
    }
    draw();
    return;
  }
  send(target);
}

void MediaRemoteScreen::onButtonA() {
  if (view_ == View::BleInfo) {
    if (manager_) manager_->show(ScreenId::MainMenu);
    return;
  }
  send(Target::Previous);
}

void MediaRemoteScreen::onButtonB() {
  if (view_ == View::BleInfo) {
    bluetooth_.setEnabled(true);
    bluetooth_.startAdvertising();
    view_ = View::Remote;
    draw();
    return;
  }
  send(Target::Next);
}

void MediaRemoteScreen::drawBleInfo() {
  const Theme theme = currentTheme(settings_);
  M5.Display.fillScreen(theme.background);
  M5.Display.setTextDatum(middle_center);
  drawButton(kBackX, kBackY, kBackW, kBackH, "Back", "", Target::Menu);
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.drawString("Pair New Device", kCenter, 62);
  M5.Display.setFont(&fonts::FreeSans12pt7b);
  M5.Display.setTextColor(theme.muted, theme.background);
  M5.Display.drawString("Iris uses Bluetooth", kCenter, 132);
  M5.Display.drawString("Low Energy (BLE).", kCenter, 164);
  M5.Display.drawString("Your phone or PC must", kCenter, 214);
  M5.Display.drawString("support BLE HID.", kCenter, 246);
  drawButton(118, 318, 230, 64, "Continue", bluetooth_.deviceName().c_str(), Target::Pair);
}

void MediaRemoteScreen::drawRemote() {
  const Theme theme = currentTheme(settings_);
  M5.Display.fillScreen(theme.background);
  M5.Display.setTextDatum(middle_center);
  drawButton(kBackX, kBackY, kBackW, kBackH, "Back", "", Target::Menu);
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.drawString("MEDIA", kCenter, 46);

  if (!bluetooth_.connected()) {
    M5.Display.drawRoundRect(132, 72, 202, 28, 14, dimColor(theme.accent));
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.setTextColor(theme.muted, theme.background);
    M5.Display.drawString(bluetooth_.deviceName(), kCenter, 86);
  }

  drawIconButton(kCenter, 178, 72, Target::PlayPause);
  drawIconButton(94, 190, 45, Target::Previous);
  drawIconButton(372, 190, 45, Target::Next);
  drawIconButton(142, 304, 44, Target::VolumeDown);
  drawIconButton(324, 304, 44, Target::VolumeUp);
  drawIconButton(kCenter, 364, 39, Target::Mute);

  M5.Display.setFont(&fonts::FreeSans9pt7b);
  const uint16_t statusColor = bluetooth_.connected() ? theme.accent : theme.muted;
  M5.Display.setTextColor(statusColor, theme.background);
  M5.Display.drawString(bluetooth_.statusText(), kCenter, 416);
  M5.Display.setTextColor(theme.muted, theme.background);
  M5.Display.drawString("A: Prev  B: Next", kCenter, 442);
}

void MediaRemoteScreen::drawButton(int32_t x, int32_t y, int32_t w, int32_t h,
                                   const char* title, const char* subtitle,
                                   Target target) {
  const Theme theme = currentTheme(settings_);
  const bool active = preview_ == target || lastSent_ == target;
  const uint16_t fill = active ? theme.selected : theme.panel;
  const uint16_t border = active ? theme.foreground : dimColor(theme.accent);
  M5.Display.fillRoundRect(x, y, w, h, 18, fill);
  M5.Display.drawRoundRect(x, y, w, h, 18, border);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSansBold12pt7b);
  M5.Display.setTextColor(theme.foreground, fill);
  M5.Display.drawString(title, x + w / 2, y + h / 2 - (subtitle[0] ? 12 : 0));
  if (subtitle[0]) {
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.setTextColor(theme.muted, fill);
    M5.Display.drawString(subtitle, x + w / 2, y + h / 2 + 18);
  }
}

void MediaRemoteScreen::drawIconButton(int32_t cx, int32_t cy, int32_t radius,
                                       Target target) {
  const Theme theme = currentTheme(settings_);
  const bool active = preview_ == target || lastSent_ == target;
  const uint16_t fill = active ? theme.selected : theme.panel;
  const uint16_t ring = active ? theme.foreground : dimColor(theme.accent);
  M5.Display.fillCircle(cx, cy, radius, fill);
  M5.Display.drawCircle(cx, cy, radius, ring);
  M5.Display.drawCircle(cx, cy, radius - 1, dimColor(ring));
  drawIcon(target, cx, cy, active ? theme.foreground : theme.accent);
}

void MediaRemoteScreen::drawIcon(Target target, int32_t cx, int32_t cy,
                                 uint16_t color) {
  switch (target) {
    case Target::PlayPause:
      M5.Display.fillTriangle(cx - 20, cy - 30, cx - 20, cy + 30, cx + 20, cy,
                              color);
      M5.Display.fillRoundRect(cx + 28, cy - 30, 10, 60, 4, color);
      M5.Display.fillRoundRect(cx + 48, cy - 30, 10, 60, 4, color);
      break;
    case Target::Previous:
      M5.Display.fillRoundRect(cx - 28, cy - 24, 8, 48, 4, color);
      M5.Display.fillTriangle(cx - 16, cy, cx + 14, cy - 24, cx + 14, cy + 24,
                              color);
      M5.Display.fillTriangle(cx + 8, cy, cx + 34, cy - 22, cx + 34, cy + 22,
                              color);
      break;
    case Target::Next:
      M5.Display.fillRoundRect(cx + 20, cy - 24, 8, 48, 4, color);
      M5.Display.fillTriangle(cx + 16, cy, cx - 14, cy - 24, cx - 14, cy + 24,
                              color);
      M5.Display.fillTriangle(cx - 8, cy, cx - 34, cy - 22, cx - 34, cy + 22,
                              color);
      break;
    case Target::VolumeDown:
      M5.Display.fillRect(cx - 27, cy - 12, 16, 24, color);
      M5.Display.fillTriangle(cx - 12, cy - 18, cx + 8, cy - 30, cx + 8, cy + 30,
                              color);
      M5.Display.fillRoundRect(cx + 18, cy - 3, 26, 6, 3, color);
      break;
    case Target::VolumeUp:
      M5.Display.fillRect(cx - 30, cy - 12, 16, 24, color);
      M5.Display.fillTriangle(cx - 15, cy - 18, cx + 5, cy - 30, cx + 5, cy + 30,
                              color);
      M5.Display.drawArc(cx + 8, cy, 18, 28, -42, 42, color);
      M5.Display.drawArc(cx + 8, cy, 30, 42, -42, 42, color);
      break;
    case Target::Mute:
      M5.Display.fillRect(cx - 30, cy - 11, 15, 22, color);
      M5.Display.fillTriangle(cx - 16, cy - 18, cx + 4, cy - 30, cx + 4, cy + 30,
                              color);
      M5.Display.drawLine(cx + 20, cy - 20, cx + 42, cy + 20, color);
      M5.Display.drawLine(cx + 42, cy - 20, cx + 20, cy + 20, color);
      M5.Display.drawLine(cx + 21, cy - 20, cx + 43, cy + 20, color);
      M5.Display.drawLine(cx + 43, cy - 20, cx + 21, cy + 20, color);
      break;
    default:
      break;
  }
}

void MediaRemoteScreen::send(Target target) {
  bool sent = false;
  switch (target) {
    case Target::Menu:
      if (manager_) manager_->show(ScreenId::MainMenu);
      return;
    case Target::Pair:
      bluetooth_.startAdvertising();
      draw();
      return;
    case Target::Previous:
      sent = bluetooth_.sendMediaCommand(BleMediaCommand::PreviousTrack);
      break;
    case Target::PlayPause:
      sent = bluetooth_.sendMediaCommand(BleMediaCommand::PlayPause);
      break;
    case Target::Next:
      sent = bluetooth_.sendMediaCommand(BleMediaCommand::NextTrack);
      break;
    case Target::VolumeDown:
      sent = bluetooth_.sendMediaCommand(BleMediaCommand::VolumeDown);
      break;
    case Target::VolumeUp:
      sent = bluetooth_.sendMediaCommand(BleMediaCommand::VolumeUp);
      break;
    case Target::Mute:
      sent = bluetooth_.sendMediaCommand(BleMediaCommand::Mute);
      break;
    default:
      return;
  }

  lastSent_ = target;
  feedbackUntilMs_ = millis() + kFeedbackMs;
  if (sent) pulseHaptic();
  draw();
}

MediaRemoteScreen::Target MediaRemoteScreen::targetAt(int32_t x, int32_t y) const {
  if (x >= kBackX - 12 && x <= kBackX + kBackW + 12 && y >= kBackY - 12 &&
      y <= kBackY + kBackH + 12) {
    return Target::Menu;
  }
  if (y >= 408 && x >= 118 && x <= 348) return Target::Menu;
  if (view_ == View::BleInfo) {
    if (x >= 118 && x <= 348 && y >= 318 && y <= 382) return Target::Pair;
    return Target::None;
  }
  const auto inCircle = [](int32_t px, int32_t py, int32_t cx, int32_t cy,
                           int32_t radius) {
    const int32_t dx = px - cx;
    const int32_t dy = py - cy;
    return dx * dx + dy * dy <= radius * radius;
  };
  if (inCircle(x, y, kCenter, 178, 84)) return Target::PlayPause;
  if (inCircle(x, y, 94, 190, 58)) return Target::Previous;
  if (inCircle(x, y, 372, 190, 58)) return Target::Next;
  if (inCircle(x, y, 142, 304, 56)) return Target::VolumeDown;
  if (inCircle(x, y, 324, 304, 56)) return Target::VolumeUp;
  if (inCircle(x, y, kCenter, 364, 52)) return Target::Mute;
  return Target::None;
}

void MediaRemoteScreen::pulseHaptic(uint8_t strength, uint32_t durationMs) {
  M5.Power.setVibration(strength);
  hapticUntilMs_ = millis() + durationMs;
}

void MediaRemoteScreen::updateHaptic(uint32_t nowMs) {
  if (hapticUntilMs_ == 0 || static_cast<int32_t>(nowMs - hapticUntilMs_) < 0) return;
  M5.Power.setVibration(0);
  hapticUntilMs_ = 0;
}

}  // namespace iris
