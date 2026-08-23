#pragma once

#include "iris/screens/Screen.h"
#include "iris/services/SettingsStore.h"
#include "iris/services/WifiService.h"

namespace iris {

class HardwareDiagnosticsScreen : public Screen {
 public:
  HardwareDiagnosticsScreen(SettingsStore& settings, WifiService& wifi)
      : settings_(settings), wifi_(wifi) {}

  void enter() override;
  void update(uint32_t nowMs) override;
  void draw() override;
  void previewTouch(int32_t x, int32_t y) override;
  void handleTouch(int32_t x, int32_t y) override;
  void onButtonA() override;
  void onButtonB() override;

 private:
  void activateSelected(uint32_t nowMs);
  void selectRow(size_t index);
  void drawRow(size_t index, bool selected);
  void drawFooter();
  int rowAt(int32_t x, int32_t y) const;
  void startAudioSilence(uint32_t nowMs);
  void testAudioTone();
  void startDisplayOff(uint32_t nowMs);
  void startBrightnessTest(uint8_t brightness, const char* label, uint32_t nowMs);
  void toggleWifi();
  void toggleWifiSleep();
  void startHapticPulse(uint32_t nowMs);
  void cyclePowerProfile();
  void restoreAudio();
  void restoreDisplay();
  void stopHaptic();
  void goBack();

  SettingsStore& settings_;
  WifiService& wifi_;
  size_t selected_ = 0;
  bool wifiSleep_ = true;
  uint32_t audioSilenceUntilMs_ = 0;
  uint32_t displayOffUntilMs_ = 0;
  uint32_t brightnessTestUntilMs_ = 0;
  uint32_t hapticUntilMs_ = 0;
  uint32_t lastStatusDrawMs_ = 0;
  const char* brightnessTestLabel_ = nullptr;
};

}  // namespace iris
