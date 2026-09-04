#include "iris/screens/RagnarLinkScreen.h"

#include "iris/Theme.h"
#include "iris/screens/ScreenManager.h"

namespace iris {

namespace {
constexpr int kCenter = 233;
constexpr int kBackX = 52;
constexpr int kBackY = 388;
constexpr int kBackW = 126;
constexpr int kBackH = 46;
constexpr int kDownX = 196;
constexpr int kUpX = 306;
constexpr int kChannelY = 388;
constexpr int kChannelW = 76;
constexpr int kChannelH = 46;
constexpr int kHitPad = 12;

uint16_t dimColor(uint16_t color) {
  const uint8_t r = (color >> 11) & 0x1F;
  const uint8_t g = (color >> 5) & 0x3F;
  const uint8_t b = color & 0x1F;
  return static_cast<uint16_t>(((r / 2) << 11) | ((g / 2) << 5) | (b / 2));
}
}  // namespace

RagnarLinkScreen::RagnarLinkScreen(SettingsStore& settings, RagnarLinkService& ragnar)
    : settings_(settings), ragnar_(ragnar), canvas_(&M5.Display) {}

void RagnarLinkScreen::enter() {
  highlightedAction_ = TouchAction::None;
  lastDrawMs_ = 0;
  lastSnapshot_ = "";
  if (!canvas_.getBuffer()) {
    canvas_.setPsram(true);
    canvas_.setColorDepth(16);
    canvasReady_ = canvas_.createSprite(M5.Display.width(), M5.Display.height()) != nullptr;
  }
  draw();
}

void RagnarLinkScreen::update(uint32_t nowMs) {
  const String snapshot = stateSnapshot();
  if (lastDrawMs_ == 0 || snapshot != lastSnapshot_ || nowMs - lastDrawMs_ >= 1000) {
    draw();
    lastDrawMs_ = nowMs;
  }
}

void RagnarLinkScreen::draw() {
  if (!canvasReady_) {
    const Theme theme = currentTheme(settings_);
    M5.Display.fillScreen(theme.background);
    return;
  }

  const Theme theme = currentTheme(settings_);
  const RagnarLinkSnapshot snapshot = ragnar_.snapshot();
  canvas_.fillScreen(theme.background);

  canvas_.setTextDatum(middle_center);
  canvas_.setFont(&fonts::FreeSansBold18pt7b);
  canvas_.setTextColor(theme.foreground, theme.background);
  canvas_.drawString("Ragnar Link", kCenter, 48);

  canvas_.setFont(&fonts::FreeSans12pt7b);
  canvas_.setTextColor(snapshot.stale ? theme.muted : theme.accent, theme.background);
  canvas_.drawString(ragnar_.statusText(millis()), kCenter, 88);

  const int panelX = 58;
  const int panelY = 116;
  const int panelW = 350;
  const int panelH = 228;
  canvas_.fillRoundRect(panelX, panelY, panelW, panelH, 18, theme.panel);
  canvas_.drawRoundRect(panelX, panelY, panelW, panelH, 18, dimColor(theme.muted));

  drawMetric(panelX + 28, panelY + 34, "State",
             RagnarLinkService::ragnarStateName(snapshot.ragnarState));
  drawMetric(panelX + 28, panelY + 70, "Capture",
             RagnarLinkService::captureStateName(snapshot.captureState));
  drawMetric(panelX + 28, panelY + 106, "GPS",
             RagnarLinkService::gpsStateName(snapshot.gpsState));
  drawMetric(panelX + 28, panelY + 142, "Cameras", String(snapshot.cameraCount));
  drawMetric(panelX + 28, panelY + 178, "WiFi / BLE",
             String(snapshot.wifiCount) + " / " + snapshot.bleCount);

  canvas_.setTextDatum(middle_center);
  canvas_.setFont(&fonts::FreeSans9pt7b);
  canvas_.setTextColor(theme.muted, theme.panel);
  String footer = String("Seq ") + snapshot.sequence + "  Up " + formatUptime(snapshot.uptimeSeconds);
  if (!snapshot.packetSeen) footer = "Waiting for Ragnar broadcast";
  canvas_.drawString(footer, kCenter, panelY + panelH - 18);

  canvas_.setTextColor(theme.muted, theme.background);
  canvas_.drawString(String("Channel ") + settings_.ragnarChannel() + " / active " +
                         (snapshot.activeChannel == 0 ? String("?") : String(snapshot.activeChannel)),
                     kCenter, 366);

  drawButton(kBackX, kBackY, kBackW, kBackH, "Back", highlightedAction_ == TouchAction::Back);
  drawButton(kDownX, kChannelY, kChannelW, kChannelH, "CH-", highlightedAction_ == TouchAction::ChannelDown);
  drawButton(kUpX, kChannelY, kChannelW, kChannelH, "CH+", highlightedAction_ == TouchAction::ChannelUp);

  canvas_.pushSprite(0, 0);
  lastSnapshot_ = stateSnapshot();
}

void RagnarLinkScreen::previewTouch(int32_t x, int32_t y) {
  const TouchAction action = actionAt(x, y);
  if (action == highlightedAction_) return;
  highlightedAction_ = action;
  draw();
}

void RagnarLinkScreen::handleTouch(int32_t x, int32_t y) {
  const TouchAction action = actionAt(x, y);
  highlightedAction_ = TouchAction::None;
  if (action == TouchAction::Back) {
    if (manager_) manager_->show(ScreenId::MainMenu);
    return;
  }
  if (action == TouchAction::ChannelDown) {
    changeChannel(-1);
    return;
  }
  if (action == TouchAction::ChannelUp) {
    changeChannel(1);
    return;
  }
  draw();
}

void RagnarLinkScreen::onButtonA() {
  changeChannel(1);
}

void RagnarLinkScreen::onButtonB() {
  if (manager_) manager_->show(ScreenId::MainMenu);
}

void RagnarLinkScreen::drawButton(int x, int y, int w, int h, const char* label,
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

void RagnarLinkScreen::drawMetric(int x, int y, const char* label, const String& value) {
  const Theme theme = currentTheme(settings_);
  canvas_.setTextDatum(middle_left);
  canvas_.setFont(&fonts::FreeSans9pt7b);
  canvas_.setTextColor(theme.muted, theme.panel);
  canvas_.drawString(label, x, y);
  canvas_.setTextDatum(middle_right);
  canvas_.setTextColor(theme.foreground, theme.panel);
  canvas_.drawString(value, 376, y);
}

void RagnarLinkScreen::changeChannel(int delta) {
  int next = static_cast<int>(settings_.ragnarChannel()) + delta;
  if (next < 1) next = 14;
  if (next > 14) next = 1;
  settings_.setRagnarChannel(static_cast<uint8_t>(next));
  M5.Power.setVibration(50);
  delay(8);
  M5.Power.setVibration(0);
  draw();
}

RagnarLinkScreen::TouchAction RagnarLinkScreen::actionAt(int32_t x, int32_t y) const {
  if (x >= kBackX - kHitPad && x <= kBackX + kBackW + kHitPad &&
      y >= kBackY - kHitPad && y <= kBackY + kBackH + kHitPad) {
    return TouchAction::Back;
  }
  if (x >= kDownX - kHitPad && x <= kDownX + kChannelW + kHitPad &&
      y >= kChannelY - kHitPad && y <= kChannelY + kChannelH + kHitPad) {
    return TouchAction::ChannelDown;
  }
  if (x >= kUpX - kHitPad && x <= kUpX + kChannelW + kHitPad &&
      y >= kChannelY - kHitPad && y <= kChannelY + kChannelH + kHitPad) {
    return TouchAction::ChannelUp;
  }
  return TouchAction::None;
}

String RagnarLinkScreen::stateSnapshot() const {
  const RagnarLinkSnapshot snapshot = ragnar_.snapshot();
  String text;
  text.reserve(160);
  text += static_cast<int>(snapshot.initialized);
  text += '|';
  text += static_cast<int>(snapshot.stale);
  text += '|';
  text += snapshot.sequence;
  text += '|';
  text += snapshot.cameraCount;
  text += '|';
  text += snapshot.wifiCount;
  text += '|';
  text += snapshot.bleCount;
  text += '|';
  text += static_cast<int>(snapshot.ragnarState);
  text += '|';
  text += static_cast<int>(snapshot.gpsState);
  text += '|';
  text += static_cast<int>(snapshot.captureState);
  text += '|';
  text += settings_.ragnarChannel();
  text += '|';
  text += static_cast<int>(highlightedAction_);
  return text;
}

String RagnarLinkScreen::formatUptime(uint32_t seconds) const {
  const uint32_t hours = seconds / 3600UL;
  const uint32_t minutes = (seconds % 3600UL) / 60UL;
  const uint32_t secs = seconds % 60UL;
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%lu:%02lu:%02lu",
           static_cast<unsigned long>(hours),
           static_cast<unsigned long>(minutes),
           static_cast<unsigned long>(secs));
  return String(buffer);
}

}  // namespace iris
