#include "iris/screens/BootloaderScreen.h"

#include <M5Unified.h>
#include <esp32-hal-tinyusb.h>

#include "iris/Theme.h"
#include "iris/screens/ScreenManager.h"

namespace iris {

namespace {
constexpr uint8_t kM5Pm1SysCmdReg = 0x0C;
constexpr uint8_t kM5Pm1DownloadModeCmd = 0xA3;
constexpr int kCancelX = 58;
constexpr int kConfirmX = 248;
constexpr int kButtonY = 314;
constexpr int kButtonWidth = 160;
constexpr int kButtonHeight = 58;
}  // namespace

void BootloaderScreen::enter() {
  rebooting_ = false;
}

void BootloaderScreen::update(uint32_t) {}

void BootloaderScreen::draw() {
  const Theme theme = currentTheme(settings_);
  M5.Display.fillScreen(theme.background);
  M5.Display.setTextDatum(middle_center);

  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.drawString(rebooting_ ? "Rebooting" : "Bootloader", M5.Display.width() / 2, 70);

  M5.Display.setFont(&fonts::FreeSans12pt7b);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.drawString("USB download mode", M5.Display.width() / 2, 138);

  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextColor(theme.muted, theme.background);
  if (rebooting_) {
    M5.Display.drawString("Keep the USB cable connected.", M5.Display.width() / 2, 210);
    return;
  }

  M5.Display.drawString("Connect USB before continuing.", M5.Display.width() / 2, 196);
  M5.Display.drawString("A: Cancel     B: Enter", M5.Display.width() / 2, 244);

  drawButton(kCancelX, kButtonY, kButtonWidth, kButtonHeight, "Cancel", theme.button, theme.foreground);
  drawButton(kConfirmX, kButtonY, kButtonWidth, kButtonHeight, "Enter", theme.selected, theme.foreground);
}

void BootloaderScreen::handleTouch(int32_t x, int32_t y) {
  if (rebooting_ || y < kButtonY || y > kButtonY + kButtonHeight) return;
  if (x >= kCancelX && x <= kCancelX + kButtonWidth) {
    goBack();
  } else if (x >= kConfirmX && x <= kConfirmX + kButtonWidth) {
    requestBootloader();
  }
}

void BootloaderScreen::onButtonA() {
  if (!rebooting_) goBack();
}

void BootloaderScreen::onButtonB() {
  if (!rebooting_) requestBootloader();
}

void BootloaderScreen::goBack() {
  if (manager_) manager_->show(ScreenId::Developer);
}

void BootloaderScreen::requestBootloader() {
  if (rebooting_) return;

  Serial.println("[BOOT] Bootloader mode requested");
  rebooting_ = true;
  draw();
  delay(300);

  Serial.println("[BOOT] Trying M5PM1 download-mode command");
  if (M5.Power.getType() == m5::Power_Class::pmic_m5pm1 &&
      M5.Power.M5pm1.writeRegister8(kM5Pm1SysCmdReg, kM5Pm1DownloadModeCmd)) {
    Serial.println("[BOOT] M5PM1 accepted download-mode command");
    Serial.flush();
    while (true) {
      delay(1000);
    }
  }

  Serial.println("[BOOT] Falling back to USB persistent bootloader restart");
  Serial.flush();
  usb_persist_restart(RESTART_BOOTLOADER);
}

void BootloaderScreen::drawButton(int x, int y, int w, int h, const char* label,
                                  uint16_t fill, uint16_t text) {
  M5.Display.fillRoundRect(x, y, w, h, 14, fill);
  M5.Display.drawRoundRect(x, y, w, h, 14, text);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSans12pt7b);
  M5.Display.setTextColor(text, fill);
  M5.Display.drawString(label, x + (w / 2), y + (h / 2));
}

}  // namespace iris
