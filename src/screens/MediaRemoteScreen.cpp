#include "iris/screens/MediaRemoteScreen.h"

#include <M5Unified.h>

#include "iris/Theme.h"
#include "iris/screens/ScreenManager.h"

namespace iris {

namespace {
constexpr int kCenter = 233;
constexpr uint32_t kFeedbackMs = 260;

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
  drawButton(156, 402, 154, 40, "Menu", "Back", Target::Menu);
}

void MediaRemoteScreen::drawRemote() {
  const Theme theme = currentTheme(settings_);
  M5.Display.fillScreen(theme.background);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.drawString("MEDIA", kCenter, 44);

  drawButton(164, 96, 138, 88, "Play", "Pause", Target::PlayPause);
  drawButton(42, 186, 122, 78, "Prev", "Track", Target::Previous);
  drawButton(302, 186, 122, 78, "Next", "Track", Target::Next);
  drawButton(92, 286, 130, 64, "Vol -", "", Target::VolumeDown);
  drawButton(244, 286, 130, 64, "Vol +", "", Target::VolumeUp);
  drawButton(156, 360, 154, 42, "Mute", "", Target::Mute);

  M5.Display.setFont(&fonts::FreeSans9pt7b);
  const uint16_t statusColor = bluetooth_.connected() ? theme.accent : theme.muted;
  M5.Display.setTextColor(statusColor, theme.background);
  M5.Display.drawString(bluetooth_.statusText(), kCenter, 420);
  M5.Display.setTextColor(theme.muted, theme.background);
  M5.Display.drawString("A: Prev  B: Next", kCenter, 446);

  if (!bluetooth_.connected()) {
    M5.Display.drawRoundRect(132, 68, 202, 28, 14, dimColor(theme.accent));
    M5.Display.setTextColor(theme.muted, theme.background);
    M5.Display.drawString(bluetooth_.deviceName(), kCenter, 82);
  }
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
  if (y >= 404) return Target::Menu;
  if (view_ == View::BleInfo) {
    if (x >= 118 && x <= 348 && y >= 318 && y <= 382) return Target::Pair;
    return Target::None;
  }
  if (x >= 164 && x <= 302 && y >= 96 && y <= 184) return Target::PlayPause;
  if (x >= 42 && x <= 164 && y >= 186 && y <= 264) return Target::Previous;
  if (x >= 302 && x <= 424 && y >= 186 && y <= 264) return Target::Next;
  if (x >= 92 && x <= 222 && y >= 286 && y <= 350) return Target::VolumeDown;
  if (x >= 244 && x <= 374 && y >= 286 && y <= 350) return Target::VolumeUp;
  if (x >= 156 && x <= 310 && y >= 360 && y <= 402) return Target::Mute;
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
