#pragma once

#include <M5Unified.h>

#include "iris/screens/Screen.h"
#include "iris/services/NetworkScanService.h"
#include "iris/services/SettingsStore.h"

namespace iris {

class WifiScannerScreen : public Screen {
 public:
  WifiScannerScreen(SettingsStore& settings, NetworkScanService& scanner);

  void enter() override;
  void update(uint32_t nowMs) override;
  void draw() override;
  void previewTouch(int32_t x, int32_t y) override;
  void handleTouch(int32_t x, int32_t y) override;
  void onButtonA() override;
  void onButtonB() override;

 private:
  enum class View : uint8_t {
    List,
    Detail,
  };

  enum class TouchAction : uint8_t {
    None,
    Back,
    Scan,
    Row0,
    Row1,
    Row2,
    Row3,
  };

  void drawList();
  void drawDetail();
  void drawHeader(const char* title);
  void drawButton(int x, int y, int w, int h, const char* label, bool highlighted);
  void drawNetworkRow(size_t visibleRow, size_t resultIndex, bool selected);
  void drawChannels();
  void drawFooter(const char* text);
  void moveSelection(int delta);
  void openSelected();
  void startScan();
  void goBack();
  void drawCurrentView();
  String stateSnapshot() const;
  TouchAction actionAt(int32_t x, int32_t y) const;
  int rowForAction(TouchAction action) const;
  String fitText(const String& text, int maxWidth);

  SettingsStore& settings_;
  NetworkScanService& scanner_;
  M5Canvas canvas_;
  bool canvasReady_ = false;
  View view_ = View::List;
  size_t selected_ = 0;
  size_t topIndex_ = 0;
  uint32_t lastDrawMs_ = 0;
  String lastSnapshot_;
  TouchAction highlightedAction_ = TouchAction::None;
};

}  // namespace iris
