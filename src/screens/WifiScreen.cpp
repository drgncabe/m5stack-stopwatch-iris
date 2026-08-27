#include "iris/screens/WifiScreen.h"

#include <M5Unified.h>
#include "iris/Theme.h"
#include "iris/screens/ScreenManager.h"

namespace iris {

namespace {
constexpr int kToggleX = 55;
constexpr int kSetupX = 241;
constexpr int kTopButtonY = 302;
constexpr int kTopButtonWidth = 170;
constexpr int kTopButtonHeight = 58;
constexpr int kScannerX = 55;
constexpr int kBackX = 241;
constexpr int kBackY = 374;
constexpr int kBottomButtonWidth = 170;
constexpr int kBackHeight = 56;
constexpr int kHitPad = 14;
}  // namespace

void WifiScreen::enter() {
  lastDrawMs_ = 0;
  lastSnapshot_ = "";
  highlightedAction_ = TouchAction::None;
}

void WifiScreen::update(uint32_t nowMs) {
  if (lastDrawMs_ == 0 || nowMs - lastDrawMs_ >= 1000) {
    lastDrawMs_ = nowMs;
    const String current = snapshot();
    if (current != lastSnapshot_) {
      drawStatus();
      drawControls(highlightedAction_);
      lastSnapshot_ = current;
    }
  }
}

void WifiScreen::draw() {
  const Theme theme = currentTheme(settings_);
  M5.Display.fillScreen(theme.background);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.drawString("WiFi", M5.Display.width() / 2, 52);

  drawStatus();
  drawControls(highlightedAction_);

  M5.Display.setTextColor(theme.muted, theme.background);
  M5.Display.drawString("A: Toggle   B: Setup", M5.Display.width() / 2, 446);
  lastSnapshot_ = snapshot();
}

void WifiScreen::previewTouch(int32_t x, int32_t y) {
  const TouchAction action = actionAt(x, y);
  if (action == highlightedAction_) return;
  highlightedAction_ = action;
  drawControls(highlightedAction_);
}

void WifiScreen::handleTouch(int32_t x, int32_t y) {
  const TouchAction action = actionAt(x, y);
  highlightedAction_ = TouchAction::None;
  switch (action) {
    case TouchAction::Toggle:
      toggleWifi();
      break;
    case TouchAction::Setup:
      startSetup();
      break;
    case TouchAction::Scanner:
      openScanner();
      break;
    case TouchAction::Back:
      goBack();
      break;
    default:
      drawControls(highlightedAction_);
      break;
  }
}

void WifiScreen::onButtonA() { toggleWifi(); }
void WifiScreen::onButtonB() { startSetup(); }

void WifiScreen::toggleWifi() {
  const bool enabled = !wifi_.isEnabled();
  settings_.setWifiEnabled(enabled);
  wifi_.setEnabled(enabled);
  drawStatus();
  drawControls(highlightedAction_);
  lastSnapshot_ = snapshot();
}

void WifiScreen::startSetup() {
  if (!wifi_.isEnabled()) {
    settings_.setWifiEnabled(true);
    wifi_.setEnabled(true);
  }
  wifi_.startProvisioning();
  drawStatus();
  drawControls(highlightedAction_);
  lastSnapshot_ = snapshot();
}

void WifiScreen::openScanner() {
  if (manager_) manager_->show(ScreenId::WifiScanner);
}

void WifiScreen::goBack() {
  if (manager_) manager_->show(ScreenId::Settings);
}

void WifiScreen::drawStatus() {
  const Theme theme = currentTheme(settings_);
  M5.Display.fillRect(34, 88, 398, 198, theme.background);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSans12pt7b);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.drawString(wifi_.statusText(), M5.Display.width() / 2, 112);

  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(theme.muted, theme.background);
  const String network = wifi_.ssid().isEmpty() ? String("No saved network") : wifi_.ssid();
  M5.Display.drawString(network, M5.Display.width() / 2, 148);
  M5.Display.drawString(wifi_.ipAddress(), M5.Display.width() / 2, 178);

  if (wifi_.isProvisioning()) {
    M5.Display.setTextColor(theme.foreground, theme.background);
    M5.Display.drawString("Connect phone to:", M5.Display.width() / 2, 218);
    M5.Display.drawString(wifi_.portalSsid(), M5.Display.width() / 2, 244);
    M5.Display.drawString("Open 192.168.4.1", M5.Display.width() / 2, 270);
  }
}

void WifiScreen::drawControls(TouchAction highlighted) {
  drawButton(kToggleX, kTopButtonY, kTopButtonWidth, kTopButtonHeight,
             wifi_.isEnabled() ? "Disable" : "Enable", highlighted == TouchAction::Toggle);
  drawButton(kSetupX, kTopButtonY, kTopButtonWidth, kTopButtonHeight, "Setup",
             highlighted == TouchAction::Setup);
  drawButton(kScannerX, kBackY, kBottomButtonWidth, kBackHeight, "Scanner",
             highlighted == TouchAction::Scanner);
  drawButton(kBackX, kBackY, kBottomButtonWidth, kBackHeight, "Back",
             highlighted == TouchAction::Back);
}

void WifiScreen::drawButton(int x, int y, int w, int h, const char* label, bool highlighted) {
  const Theme theme = currentTheme(settings_);
  const uint16_t fill = highlighted ? theme.selected : theme.button;
  const uint16_t border = highlighted ? theme.foreground : theme.panel;
  M5.Display.fillRoundRect(x, y, w, h, 16, fill);
  M5.Display.drawRoundRect(x, y, w, h, 16, border);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(theme.foreground, fill);
  M5.Display.drawString(label, x + (w / 2), y + (h / 2));
}

WifiScreen::TouchAction WifiScreen::actionAt(int32_t x, int32_t y) const {
  if (y >= kTopButtonY - kHitPad && y <= kTopButtonY + kTopButtonHeight + kHitPad) {
    if (x >= kToggleX - kHitPad && x <= kToggleX + kTopButtonWidth + kHitPad) {
      return TouchAction::Toggle;
    }
    if (x >= kSetupX - kHitPad && x <= kSetupX + kTopButtonWidth + kHitPad) {
      return TouchAction::Setup;
    }
  }
  if (x >= kScannerX - kHitPad && x <= kScannerX + kBottomButtonWidth + kHitPad &&
      y >= kBackY - kHitPad && y <= kBackY + kBackHeight + kHitPad) {
    return TouchAction::Scanner;
  }
  if (x >= kBackX - kHitPad && x <= kBackX + kBottomButtonWidth + kHitPad &&
      y >= kBackY - kHitPad && y <= kBackY + kBackHeight + kHitPad) {
    return TouchAction::Back;
  }
  return TouchAction::None;
}

String WifiScreen::snapshot() const {
  String text;
  text.reserve(160);
  text += wifi_.statusText();
  text += '|';
  text += wifi_.ssid();
  text += '|';
  text += wifi_.ipAddress();
  text += '|';
  text += wifi_.portalSsid();
  text += '|';
  text += wifi_.isEnabled() ? '1' : '0';
  text += wifi_.isProvisioning() ? '1' : '0';
  text += settings_.watchBackground();
  return text;
}

}  // namespace iris
