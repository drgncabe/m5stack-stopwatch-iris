#pragma once

#include <Arduino.h>

#include "iris/screens/Screen.h"
#include "iris/services/BluetoothService.h"
#include "iris/services/SettingsStore.h"

namespace iris {

class MediaRemoteScreen : public Screen {
 public:
  MediaRemoteScreen(SettingsStore& settings, BluetoothService& bluetooth);

  void enter() override;
  void update(uint32_t nowMs) override;
  void draw() override;
  void previewTouch(int32_t x, int32_t y) override;
  void handleTouch(int32_t x, int32_t y) override;
  void onButtonA() override;
  void onButtonB() override;

 private:
  enum class View : uint8_t {
    BleInfo,
    Remote,
  };

  enum class Target : uint8_t {
    None,
    Menu,
    Pair,
    Previous,
    PlayPause,
    Next,
    VolumeDown,
    VolumeUp,
    Mute,
  };

  void drawBleInfo();
  void drawRemote();
  void drawButton(int32_t x, int32_t y, int32_t w, int32_t h, const char* title,
                  const char* subtitle, Target target);
  void send(Target target);
  Target targetAt(int32_t x, int32_t y) const;
  void pulseHaptic(uint8_t strength = 70, uint32_t durationMs = 12);
  void updateHaptic(uint32_t nowMs);

  SettingsStore& settings_;
  BluetoothService& bluetooth_;
  View view_ = View::BleInfo;
  Target preview_ = Target::None;
  Target lastSent_ = Target::None;
  uint32_t feedbackUntilMs_ = 0;
  uint32_t hapticUntilMs_ = 0;
};

}  // namespace iris
