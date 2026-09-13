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
constexpr int kForgetX = 346;
constexpr int kForgetY = 24;
constexpr int kForgetW = 92;
constexpr int kForgetH = 44;
constexpr int kAdvertiseX = 136;
constexpr int kAdvertiseY = 104;
constexpr int kAdvertiseW = 194;
constexpr int kAdvertiseH = 36;
constexpr uint32_t kStateRefreshMs = 250;

uint16_t dimColor(uint16_t color) {
  const uint8_t r = (color >> 11) & 0x1F;
  const uint8_t g = (color >> 5) & 0x3F;
  const uint8_t b = color & 0x1F;
  return static_cast<uint16_t>(((r / 2) << 11) | ((g / 2) << 5) | (b / 2));
}

String shortBleLabel(const String& value) {
  if (value.length() <= 20) return value;
  return value.substring(0, 8) + "..." + value.substring(value.length() - 9);
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
  lastStateCheckMs_ = 0;
  captureStateToken();
  draw();
}

void MediaRemoteScreen::update(uint32_t nowMs) {
  updateHaptic(nowMs);
  if (lastStateCheckMs_ == 0 || nowMs - lastStateCheckMs_ >= kStateRefreshMs) {
    lastStateCheckMs_ = nowMs;
    const String token = stateToken();
    if (token != lastStateToken_) {
      lastStateToken_ = token;
      if (view_ == View::BleInfo &&
          (bluetooth_.bondedDeviceCount() > 0 || bluetooth_.connected())) {
        view_ = View::Remote;
      }
      draw();
    }
  }
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
  if (view_ == View::BleInfo || !bluetooth_.connected()) {
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
  if (!bluetooth_.connected()) {
    toggleAdvertising();
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
  if (!bluetooth_.connected() && bluetooth_.bondedDeviceCount() > 0) {
    drawButton(kForgetX, kForgetY, kForgetW, kForgetH, "Forget", "", Target::Forget);
  }
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.drawString("MEDIA", kCenter, 46);

  const bool connected = bluetooth_.connected();
  const String remoteLabel = bluetooth_.connected()
                                 ? bluetooth_.activeDevice()
                                 : bluetooth_.bondedDeviceSummary();
  if (connected || bluetooth_.bondedDeviceCount() > 0) {
    M5.Display.drawRoundRect(132, 72, 202, 28, 14, dimColor(theme.accent));
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.setTextColor(theme.muted, theme.background);
    M5.Display.drawString(shortBleLabel(remoteLabel), kCenter, 86);
  } else {
    M5.Display.drawRoundRect(132, 72, 202, 28, 14, dimColor(theme.accent));
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.setTextColor(theme.muted, theme.background);
    M5.Display.drawString(bluetooth_.deviceName(), kCenter, 86);
  }

  if (!connected) {
    drawButton(kAdvertiseX, kAdvertiseY, kAdvertiseW, kAdvertiseH,
               bluetooth_.advertising() ? "Cancel" : "Advertise",
               bluetooth_.advertising() ? "Advertising" : bluetooth_.deviceName().c_str(),
               bluetooth_.advertising() ? Target::StopPairing : Target::Pair);
  }

  drawIconButton(kCenter, connected ? 186 : 204, connected ? 64 : 54, Target::PlayPause, connected);
  drawIconButton(94, connected ? 198 : 214, connected ? 41 : 38, Target::Previous, connected);
  drawIconButton(372, connected ? 198 : 214, connected ? 41 : 38, Target::Next, connected);
  drawIconButton(142, 306, 40, Target::VolumeDown, connected);
  drawIconButton(324, 306, 40, Target::VolumeUp, connected);
  drawIconButton(kCenter, 364, 36, Target::Mute, connected);

  M5.Display.setFont(&fonts::FreeSans9pt7b);
  const bool showingFeedback = lastSent_ != Target::None && feedbackUntilMs_ != 0;
  const uint16_t statusColor = showingFeedback ? theme.foreground :
                               (connected ? theme.accent : theme.muted);
  M5.Display.setTextColor(statusColor, theme.background);
  M5.Display.drawString(showingFeedback ? targetLabel(lastSent_) : bluetooth_.statusText(),
                        kCenter, 416);
  M5.Display.setTextColor(theme.muted, theme.background);
  M5.Display.drawString(connected ? "A: Prev  B: Next" :
                        (bluetooth_.advertising() ? "A: Menu  B: Cancel" :
                                                     "A: Menu  B: Advertise"),
                        kCenter, 442);
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
                                       Target target, bool enabled) {
  const Theme theme = currentTheme(settings_);
  const bool active = enabled && (preview_ == target || lastSent_ == target);
  const uint16_t fill = active ? theme.selected : theme.panel;
  const uint16_t ring = active ? theme.foreground : dimColor(theme.accent);
  const uint16_t icon = enabled ? (active ? theme.foreground : theme.accent) : dimColor(theme.muted);
  M5.Display.fillCircle(cx, cy, radius, fill);
  M5.Display.drawCircle(cx, cy, radius, ring);
  M5.Display.drawCircle(cx, cy, radius - 1, dimColor(ring));
  drawIcon(target, cx, cy, icon);
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
      toggleAdvertising();
      return;
    case Target::StopPairing:
      toggleAdvertising();
      return;
    case Target::Forget:
      bluetooth_.forgetBondedDevices();
      view_ = View::BleInfo;
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

  if (!sent) {
    lastSent_ = Target::None;
    feedbackUntilMs_ = 0;
    draw();
    return;
  }
  lastSent_ = target;
  feedbackUntilMs_ = millis() + kFeedbackMs;
  pulseHaptic();
  draw();
}

void MediaRemoteScreen::toggleAdvertising() {
  if (bluetooth_.advertising()) {
    bluetooth_.stopAdvertising();
    lastSent_ = Target::StopPairing;
  } else {
    bluetooth_.setEnabled(true);
    bluetooth_.startAdvertising();
    lastSent_ = Target::Pair;
  }
  feedbackUntilMs_ = millis() + kFeedbackMs;
  draw();
}

bool MediaRemoteScreen::isMediaTarget(Target target) const {
  return target == Target::Previous || target == Target::PlayPause ||
         target == Target::Next || target == Target::VolumeDown ||
         target == Target::VolumeUp || target == Target::Mute;
}

String MediaRemoteScreen::stateToken() const {
  String token;
  token.reserve(96);
  token += bluetooth_.enabled() ? '1' : '0';
  token += bluetooth_.initialized() ? '1' : '0';
  token += bluetooth_.advertising() ? '1' : '0';
  token += bluetooth_.connected() ? '1' : '0';
  token += bluetooth_.pairingRequested() ? '1' : '0';
  token += bluetooth_.pairingFailed() ? '1' : '0';
  token += ':';
  token += bluetooth_.activeDevice();
  token += ':';
  token += bluetooth_.bondedDeviceSummary();
  token += ':';
  token += bluetooth_.statusText();
  return token;
}

void MediaRemoteScreen::captureStateToken() {
  lastStateToken_ = stateToken();
}

const char* MediaRemoteScreen::targetLabel(Target target) const {
  switch (target) {
    case Target::Pair: return "Advertising";
    case Target::StopPairing: return "Advertising stopped";
    case Target::Previous: return "Previous sent";
    case Target::PlayPause: return "Play/Pause sent";
    case Target::Next: return "Next sent";
    case Target::VolumeDown: return "Volume down sent";
    case Target::VolumeUp: return "Volume up sent";
    case Target::Mute: return "Mute sent";
    default: return "";
  }
}

MediaRemoteScreen::Target MediaRemoteScreen::targetAt(int32_t x, int32_t y) const {
  if (x >= kBackX - 12 && x <= kBackX + kBackW + 12 && y >= kBackY - 12 &&
      y <= kBackY + kBackH + 12) {
    return Target::Menu;
  }
  if (view_ == View::Remote && !bluetooth_.connected() && bluetooth_.bondedDeviceCount() > 0 &&
      x >= kForgetX - 12 && x <= kForgetX + kForgetW + 12 &&
      y >= kForgetY - 12 && y <= kForgetY + kForgetH + 12) {
    return Target::Forget;
  }
  if (view_ == View::Remote && !bluetooth_.connected() &&
      x >= kAdvertiseX - 12 && x <= kAdvertiseX + kAdvertiseW + 12 &&
      y >= kAdvertiseY - 12 && y <= kAdvertiseY + kAdvertiseH + 12) {
    return bluetooth_.advertising() ? Target::StopPairing : Target::Pair;
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
  Target target = Target::None;
  if (inCircle(x, y, kCenter, bluetooth_.connected() ? 186 : 204,
               bluetooth_.connected() ? 76 : 66)) target = Target::PlayPause;
  if (inCircle(x, y, 94, bluetooth_.connected() ? 198 : 214,
               bluetooth_.connected() ? 52 : 48)) target = Target::Previous;
  if (inCircle(x, y, 372, bluetooth_.connected() ? 198 : 214,
               bluetooth_.connected() ? 52 : 48)) target = Target::Next;
  if (inCircle(x, y, 142, 306, 52)) target = Target::VolumeDown;
  if (inCircle(x, y, 324, 306, 52)) target = Target::VolumeUp;
  if (inCircle(x, y, kCenter, 364, 48)) target = Target::Mute;
  return !bluetooth_.connected() && isMediaTarget(target) ? Target::None : target;
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
