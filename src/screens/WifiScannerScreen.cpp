#include "iris/screens/WifiScannerScreen.h"

#include <M5Unified.h>

#include "iris/Theme.h"
#include "iris/screens/ScreenManager.h"

namespace iris {

namespace {
constexpr int kCenter = 233;
constexpr int kScanX = 296;
constexpr int kScanY = 28;
constexpr int kScanW = 116;
constexpr int kScanH = 42;
constexpr int kBackX = 54;
constexpr int kBackY = 384;
constexpr int kBackW = 144;
constexpr int kBackH = 48;
constexpr int kRowX = 44;
constexpr int kRowY = 132;
constexpr int kRowW = 378;
constexpr int kRowH = 54;
constexpr int kRowGap = 8;
constexpr int kHitPad = 12;
constexpr size_t kVisibleRows = 4;

uint16_t dimColor(uint16_t color) {
  const uint8_t r = (color >> 11) & 0x1F;
  const uint8_t g = (color >> 5) & 0x3F;
  const uint8_t b = color & 0x1F;
  return static_cast<uint16_t>(((r / 2) << 11) | ((g / 2) << 5) | (b / 2));
}

const char* hiddenText(bool hidden) {
  return hidden ? "Hidden" : "Visible";
}
}  // namespace

WifiScannerScreen::WifiScannerScreen(SettingsStore& settings, NetworkScanService& scanner)
    : settings_(settings), scanner_(scanner), canvas_(&M5.Display) {}

void WifiScannerScreen::enter() {
  view_ = View::List;
  selected_ = 0;
  topIndex_ = 0;
  highlightedAction_ = TouchAction::None;
  lastDrawMs_ = 0;
  lastSnapshot_ = "";
  if (!canvas_.getBuffer()) {
    canvas_.setPsram(true);
    canvas_.setColorDepth(16);
    canvasReady_ = canvas_.createSprite(M5.Display.width(), M5.Display.height()) != nullptr;
  }
  if (scanner_.state() == NetworkScanState::Idle) {
    scanner_.startScan();
  }
  draw();
}

void WifiScannerScreen::update(uint32_t nowMs) {
  scanner_.update(nowMs);
  const String snapshot = stateSnapshot();
  if (lastDrawMs_ == 0 || snapshot != lastSnapshot_ || nowMs - lastDrawMs_ >= 1000) {
    draw();
    lastDrawMs_ = nowMs;
  }
}

void WifiScannerScreen::draw() {
  if (!canvasReady_) {
    const Theme theme = currentTheme(settings_);
    M5.Display.fillScreen(theme.background);
    return;
  }
  drawCurrentView();
  lastSnapshot_ = stateSnapshot();
}

void WifiScannerScreen::previewTouch(int32_t x, int32_t y) {
  const TouchAction action = actionAt(x, y);
  if (action == highlightedAction_) return;
  highlightedAction_ = action;
  drawCurrentView();
}

void WifiScannerScreen::handleTouch(int32_t x, int32_t y) {
  const TouchAction action = actionAt(x, y);
  highlightedAction_ = TouchAction::None;

  if (action == TouchAction::Back) {
    goBack();
    return;
  }
  if (action == TouchAction::Scan) {
    startScan();
    return;
  }

  const int row = rowForAction(action);
  if (view_ == View::List && row >= 0) {
    const size_t index = topIndex_ + static_cast<size_t>(row);
    if (index < scanner_.resultCount()) {
      selected_ = index;
      openSelected();
      return;
    }
  }
  drawCurrentView();
}

void WifiScannerScreen::onButtonA() {
  if (view_ == View::Detail) {
    view_ = View::List;
    draw();
    return;
  }
  moveSelection(1);
}

void WifiScannerScreen::onButtonB() {
  if (view_ == View::Detail) {
    startScan();
    return;
  }
  if (scanner_.resultCount() == 0 || scanner_.scanning()) {
    startScan();
    return;
  }
  openSelected();
}

void WifiScannerScreen::drawList() {
  const Theme theme = currentTheme(settings_);
  canvas_.fillScreen(theme.background);
  drawHeader("WiFi Scanner");

  canvas_.setTextDatum(middle_center);
  canvas_.setFont(&fonts::FreeSans9pt7b);
  canvas_.setTextColor(theme.muted, theme.background);
  canvas_.drawString(String(scanner_.sourceName()) + " / " + scanner_.bandName(), kCenter, 92);

  if (scanner_.state() == NetworkScanState::Unavailable) {
    canvas_.setFont(&fonts::FreeSans12pt7b);
    canvas_.setTextColor(theme.foreground, theme.background);
    canvas_.drawString("Portal is running", kCenter, 190);
    canvas_.setFont(&fonts::FreeSans9pt7b);
    canvas_.setTextColor(theme.muted, theme.background);
    canvas_.drawString("Stop WiFi setup before scanning", kCenter, 226);
  } else if (scanner_.scanning()) {
    canvas_.setFont(&fonts::FreeSans12pt7b);
    canvas_.setTextColor(theme.accent, theme.background);
    canvas_.drawString("Scanning 2.4 GHz...", kCenter, 202);
  } else if (scanner_.resultCount() == 0) {
    canvas_.setFont(&fonts::FreeSans12pt7b);
    canvas_.setTextColor(theme.foreground, theme.background);
    canvas_.drawString(scanner_.stateText(), kCenter, 188);
    canvas_.setFont(&fonts::FreeSans9pt7b);
    canvas_.setTextColor(theme.muted, theme.background);
    canvas_.drawString("No networks shown", kCenter, 224);
  } else {
    for (size_t row = 0; row < kVisibleRows; ++row) {
      const size_t index = topIndex_ + row;
      if (index >= scanner_.resultCount()) break;
      drawNetworkRow(row, index, index == selected_);
    }
  }

  drawChannels();
  drawButton(kBackX, kBackY, kBackW, kBackH, "Back", highlightedAction_ == TouchAction::Back);
  drawButton(kScanX, kScanY, kScanW, kScanH, scanner_.scanning() ? "..." : "Scan",
             highlightedAction_ == TouchAction::Scan);
  drawFooter("A: Next   B: Details");
}

void WifiScannerScreen::drawDetail() {
  const Theme theme = currentTheme(settings_);
  canvas_.fillScreen(theme.background);
  drawHeader("Network");
  drawButton(kBackX, kBackY, kBackW, kBackH, "Back", highlightedAction_ == TouchAction::Back);
  drawButton(kScanX, kScanY, kScanW, kScanH, "Scan", highlightedAction_ == TouchAction::Scan);

  const WifiScanResult* result = scanner_.resultAt(selected_);
  if (!result) {
    canvas_.setTextDatum(middle_center);
    canvas_.setFont(&fonts::FreeSans12pt7b);
    canvas_.setTextColor(theme.muted, theme.background);
    canvas_.drawString("No network selected", kCenter, 220);
    drawFooter("A: Back   B: Scan");
    return;
  }

  canvas_.setTextDatum(middle_center);
  canvas_.setFont(&fonts::FreeSansBold12pt7b);
  canvas_.setTextColor(theme.foreground, theme.background);
  canvas_.drawString(fitText(result->ssid, 342), kCenter, 96);

  canvas_.setFont(&fonts::FreeSans9pt7b);
  canvas_.setTextColor(theme.muted, theme.background);
  canvas_.drawString(String("BSSID ") + result->bssid, kCenter, 130);

  const int panelX = 60;
  const int panelY = 166;
  const int panelW = 346;
  const int panelH = 178;
  canvas_.fillRoundRect(panelX, panelY, panelW, panelH, 18, theme.panel);
  canvas_.drawRoundRect(panelX, panelY, panelW, panelH, 18, dimColor(theme.muted));

  canvas_.setTextDatum(middle_left);
  canvas_.setTextColor(theme.foreground, theme.panel);
  canvas_.drawString(String("Signal  ") + result->rssi + " dBm", panelX + 32, panelY + 36);
  canvas_.drawString(String("Channel ") + result->channel, panelX + 32, panelY + 74);
  canvas_.drawString(String("Security ") + NetworkScanService::securityName(result->security),
                     panelX + 32, panelY + 112);
  canvas_.drawString(String("SSID    ") + hiddenText(result->hidden), panelX + 32, panelY + 150);

  canvas_.setTextDatum(middle_center);
  canvas_.setTextColor(theme.muted, theme.background);
  canvas_.drawString("Does not connect or save networks", kCenter, 366);
  drawFooter("A: Back   B: Scan");
}

void WifiScannerScreen::drawHeader(const char* title) {
  const Theme theme = currentTheme(settings_);
  canvas_.setTextDatum(middle_center);
  canvas_.setFont(&fonts::FreeSansBold12pt7b);
  canvas_.setTextColor(theme.foreground, theme.background);
  canvas_.drawString(title, kCenter, 52);

  canvas_.setFont(&fonts::FreeSans9pt7b);
  canvas_.setTextColor(theme.accent, theme.background);
  String summary = String(scanner_.detectedCount()) + " found";
  if (scanner_.truncated()) {
    summary += String(" / showing ") + scanner_.resultCount();
  }
  if (scanner_.lastScanMs() > 0) {
    summary += String(" / ") + ((millis() - scanner_.lastScanMs()) / 1000UL) + "s ago";
  }
  canvas_.drawString(summary, kCenter, 76);
}

void WifiScannerScreen::drawButton(int x, int y, int w, int h, const char* label,
                                   bool highlighted) {
  const Theme theme = currentTheme(settings_);
  const uint16_t fill = highlighted ? theme.selected : theme.button;
  const uint16_t border = highlighted ? theme.foreground : theme.panel;
  canvas_.fillRoundRect(x, y, w, h, 16, fill);
  canvas_.drawRoundRect(x, y, w, h, 16, border);
  canvas_.setTextDatum(middle_center);
  canvas_.setFont(&fonts::FreeSans9pt7b);
  canvas_.setTextColor(theme.foreground, fill);
  canvas_.drawString(label, x + (w / 2), y + (h / 2));
}

void WifiScannerScreen::drawNetworkRow(size_t visibleRow, size_t resultIndex, bool selected) {
  const Theme theme = currentTheme(settings_);
  const WifiScanResult* result = scanner_.resultAt(resultIndex);
  if (!result) return;

  const int y = kRowY + static_cast<int>(visibleRow) * (kRowH + kRowGap);
  const uint16_t fill = selected ? theme.selected : theme.panel;
  canvas_.fillRoundRect(kRowX, y, kRowW, kRowH, 14, fill);
  if (selected) canvas_.drawRoundRect(kRowX, y, kRowW, kRowH, 14, theme.accent);

  canvas_.setTextDatum(middle_left);
  canvas_.setFont(&fonts::FreeSans9pt7b);
  canvas_.setTextColor(theme.foreground, fill);
  canvas_.drawString(fitText(result->ssid, 210), kRowX + 22, y + 18);
  canvas_.setTextColor(theme.muted, fill);
  canvas_.drawString(String(result->rssi) + " dBm  CH " + result->channel, kRowX + 22, y + 40);
  canvas_.setTextDatum(middle_right);
  canvas_.setTextColor(theme.accent, fill);
  canvas_.drawString(NetworkScanService::securityName(result->security), kRowX + kRowW - 18,
                     y + 28);
}

void WifiScannerScreen::drawChannels() {
  if (scanner_.resultCount() == 0 || scanner_.scanning()) return;

  const Theme theme = currentTheme(settings_);
  canvas_.setTextDatum(middle_center);
  canvas_.setFont(&fonts::FreeSans9pt7b);
  canvas_.setTextColor(theme.muted, theme.background);
  canvas_.drawString("Channels", kCenter, 346);

  uint8_t bestChannels[3] = {0, 0, 0};
  uint8_t bestCounts[3] = {0, 0, 0};
  for (uint8_t channel = 1; channel <= 14; ++channel) {
    const uint8_t count = scanner_.channelUse(channel);
    for (uint8_t slot = 0; slot < 3; ++slot) {
      if (count > bestCounts[slot]) {
        for (int move = 2; move > static_cast<int>(slot); --move) {
          bestCounts[move] = bestCounts[move - 1];
          bestChannels[move] = bestChannels[move - 1];
        }
        bestCounts[slot] = count;
        bestChannels[slot] = channel;
        break;
      }
    }
  }

  canvas_.setTextColor(theme.foreground, theme.background);
  for (uint8_t i = 0; i < 3; ++i) {
    if (bestCounts[i] == 0) continue;
    canvas_.drawString(String("CH ") + bestChannels[i] + "  " + bestCounts[i],
                       148 + static_cast<int>(i) * 86, 368);
  }
}

void WifiScannerScreen::drawFooter(const char* text) {
  const Theme theme = currentTheme(settings_);
  canvas_.setTextDatum(middle_center);
  canvas_.setFont(&fonts::FreeSans9pt7b);
  canvas_.setTextColor(theme.muted, theme.background);
  canvas_.drawString(text, kCenter, 446);
  canvas_.pushSprite(0, 0);
}

void WifiScannerScreen::moveSelection(int delta) {
  const size_t count = scanner_.resultCount();
  if (count == 0) return;
  const int next = constrain(static_cast<int>(selected_) + delta, 0, static_cast<int>(count) - 1);
  selected_ = static_cast<size_t>(next);
  if (selected_ < topIndex_) topIndex_ = selected_;
  if (selected_ >= topIndex_ + kVisibleRows) topIndex_ = selected_ - kVisibleRows + 1;
  M5.Power.setVibration(60);
  delay(10);
  M5.Power.setVibration(0);
  draw();
}

void WifiScannerScreen::openSelected() {
  if (scanner_.resultCount() == 0) return;
  view_ = View::Detail;
  draw();
}

void WifiScannerScreen::startScan() {
  view_ = View::List;
  selected_ = 0;
  topIndex_ = 0;
  scanner_.startScan();
  draw();
}

void WifiScannerScreen::goBack() {
  if (view_ == View::Detail) {
    view_ = View::List;
    draw();
    return;
  }
  scanner_.cancelScan();
  if (manager_) manager_->show(ScreenId::MainMenu);
}

void WifiScannerScreen::drawCurrentView() {
  if (view_ == View::Detail) {
    drawDetail();
  } else {
    drawList();
  }
}

String WifiScannerScreen::stateSnapshot() const {
  String snapshot;
  snapshot.reserve(160);
  snapshot += static_cast<int>(scanner_.state());
  snapshot += '|';
  snapshot += scanner_.detectedCount();
  snapshot += '|';
  snapshot += scanner_.resultCount();
  snapshot += '|';
  snapshot += selected_;
  snapshot += '|';
  snapshot += topIndex_;
  snapshot += '|';
  snapshot += scanner_.lastScanMs();
  snapshot += '|';
  snapshot += static_cast<int>(view_);
  snapshot += '|';
  snapshot += static_cast<int>(highlightedAction_);
  return snapshot;
}

WifiScannerScreen::TouchAction WifiScannerScreen::actionAt(int32_t x, int32_t y) const {
  if (x >= kScanX - kHitPad && x <= kScanX + kScanW + kHitPad &&
      y >= kScanY - kHitPad && y <= kScanY + kScanH + kHitPad) {
    return TouchAction::Scan;
  }
  if (x >= kBackX - kHitPad && x <= kBackX + kBackW + kHitPad &&
      y >= kBackY - kHitPad && y <= kBackY + kBackH + kHitPad) {
    return TouchAction::Back;
  }
  if (view_ != View::List) return TouchAction::None;
  for (size_t row = 0; row < kVisibleRows; ++row) {
    const int rowY = kRowY + static_cast<int>(row) * (kRowH + kRowGap);
    if (x >= kRowX - kHitPad && x <= kRowX + kRowW + kHitPad &&
        y >= rowY - kHitPad && y <= rowY + kRowH + kHitPad) {
      return static_cast<TouchAction>(static_cast<uint8_t>(TouchAction::Row0) + row);
    }
  }
  return TouchAction::None;
}

int WifiScannerScreen::rowForAction(TouchAction action) const {
  const uint8_t value = static_cast<uint8_t>(action);
  const uint8_t first = static_cast<uint8_t>(TouchAction::Row0);
  if (value < first || value > static_cast<uint8_t>(TouchAction::Row3)) return -1;
  return static_cast<int>(value - first);
}

String WifiScannerScreen::fitText(const String& text, int maxWidth) {
  String fitted = text;
  if (maxWidth <= 0 || canvas_.textWidth(fitted) <= maxWidth) return fitted;
  const String ellipsis("...");
  while (fitted.length() > 0 && canvas_.textWidth(fitted + ellipsis) > maxWidth) {
    fitted.remove(fitted.length() - 1);
  }
  return fitted.length() > 0 ? fitted + ellipsis : ellipsis;
}

}  // namespace iris
