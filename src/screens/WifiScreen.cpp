#include "iris/screens/WifiScreen.h"

#include <M5Unified.h>
#include "iris/Theme.h"
#include "iris/screens/ScreenManager.h"

namespace iris {

void WifiScreen::enter() {
  lastDrawMs_ = 0;
  lastSnapshot_ = "";
}

void WifiScreen::update(uint32_t nowMs) {
  if (lastDrawMs_ == 0 || nowMs - lastDrawMs_ >= 1000) {
    lastDrawMs_ = nowMs;
    const String current = snapshot();
    if (current != lastSnapshot_) {
      draw();
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

  M5.Display.setFont(&fonts::FreeSans12pt7b);
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

  M5.Display.fillRoundRect(55, 302, 170, 58, 16, theme.button);
  M5.Display.fillRoundRect(241, 302, 170, 58, 16, theme.button);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(theme.foreground, theme.button);
  M5.Display.drawString(wifi_.isEnabled() ? "Disable" : "Enable", 140, 331);
  M5.Display.drawString("Setup", 326, 331);

  M5.Display.fillRoundRect(138, 378, 190, 48, 16, theme.button);
  M5.Display.drawString("Back", M5.Display.width() / 2, 402);

  M5.Display.setTextColor(theme.muted, theme.background);
  M5.Display.drawString("A: Toggle   B: Setup", M5.Display.width() / 2, 446);
  lastSnapshot_ = snapshot();
}

void WifiScreen::handleTouch(int32_t x, int32_t y) {
  if (y >= 292 && y <= 370) {
    if (x >= 45 && x <= 230) toggleWifi();
    if (x >= 235 && x <= 420) startSetup();
  } else if (y >= 370 && y <= 435 && x >= 115 && x <= 350) {
    goBack();
  }
}

void WifiScreen::onButtonA() { toggleWifi(); }
void WifiScreen::onButtonB() { startSetup(); }

void WifiScreen::toggleWifi() {
  const bool enabled = !wifi_.isEnabled();
  settings_.setWifiEnabled(enabled);
  wifi_.setEnabled(enabled);
  draw();
}

void WifiScreen::startSetup() {
  if (!wifi_.isEnabled()) {
    settings_.setWifiEnabled(true);
    wifi_.setEnabled(true);
  }
  wifi_.startProvisioning();
  draw();
}

void WifiScreen::goBack() {
  if (manager_) manager_->show(ScreenId::Settings);
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
