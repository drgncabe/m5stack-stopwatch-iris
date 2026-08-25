#pragma once

#include "iris/screens/Screen.h"
#include "iris/services/BatteryService.h"
#include "iris/services/BluetoothService.h"
#include "iris/services/OrientationService.h"
#include "iris/services/PowerManager.h"
#include "iris/services/SettingsStore.h"
#include "iris/services/TimeService.h"
#include "iris/services/WifiService.h"

namespace iris {

class HardwareDiagnosticsScreen : public Screen {
 public:
  HardwareDiagnosticsScreen(SettingsStore& settings, WifiService& wifi,
                            BluetoothService& bluetooth, BatteryService& battery,
                            TimeService& timeService, PowerManager& power,
                            OrientationService& orientation)
      : settings_(settings),
        wifi_(wifi),
        bluetooth_(bluetooth),
        battery_(battery),
        timeService_(timeService),
        power_(power),
        orientation_(orientation) {}

  void enter() override;
  void update(uint32_t nowMs) override;
  void draw() override;
  void previewTouch(int32_t x, int32_t y) override;
  void handleTouch(int32_t x, int32_t y) override;
  void onButtonA() override;
  void onButtonB() override;

 private:
  enum class Page : uint8_t {
    System,
    Memory,
    Display,
    Audio,
    Input,
    Imu,
    Wifi,
    Bluetooth,
    Power,
    Rtc,
    Haptics,
  };

  void activateSelected(uint32_t nowMs);
  void selectRow(size_t index);
  void nextPage();
  void drawRow(size_t index, bool selected);
  void drawLiveValues();
  void drawRowValue(size_t index);
  void drawFooter();
  int rowAt(int32_t x, int32_t y) const;
  const char* pageName() const;
  const char* rowLabel(size_t index) const;
  String rowValue(size_t index) const;
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
  BluetoothService& bluetooth_;
  BatteryService& battery_;
  TimeService& timeService_;
  PowerManager& power_;
  OrientationService& orientation_;
  Page page_ = Page::System;
  size_t selected_ = 0;
  bool wifiSleep_ = true;
  int32_t lastTouchX_ = -1;
  int32_t lastTouchY_ = -1;
  uint32_t audioSilenceUntilMs_ = 0;
  uint32_t displayOffUntilMs_ = 0;
  uint32_t brightnessTestUntilMs_ = 0;
  uint32_t hapticUntilMs_ = 0;
  uint32_t lastStatusDrawMs_ = 0;
  const char* brightnessTestLabel_ = nullptr;
};

}  // namespace iris
