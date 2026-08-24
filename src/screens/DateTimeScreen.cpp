#include "iris/screens/DateTimeScreen.h"

#include <M5Unified.h>

#include "iris/Theme.h"
#include "iris/screens/ScreenManager.h"

namespace iris {

namespace {
constexpr int kRowHeight = 32;
constexpr int kRowStartY = 70;
constexpr int kRowLeft = 42;
constexpr int kRowWidth = 382;
constexpr int kRowRectHeight = 27;
constexpr size_t kSettingsItemCount = 10;
constexpr size_t kRtcInfoItemCount = 9;
constexpr uint32_t kRefreshMs = 1000;

String fitTextToWidth(const String& text, int maxWidth) {
  if (maxWidth <= 0 || M5.Display.textWidth(text) <= maxWidth) return text;

  String fitted(text);
  const String ellipsis("...");
  while (fitted.length() > 0 &&
         M5.Display.textWidth(fitted + ellipsis) > maxWidth) {
    fitted.remove(fitted.length() - 1);
  }

  if (fitted.length() == 0) return ellipsis;
  return fitted + ellipsis;
}
}  // namespace

void DateTimeScreen::enter() {
  page_ = Page::Settings;
  selected_ = 0;
  lastRefreshMs_ = 0;
  status_ = "";
}

void DateTimeScreen::update(uint32_t nowMs) {
  if (page_ != Page::RtcInfo) return;
  if (lastRefreshMs_ == 0 || nowMs - lastRefreshMs_ >= kRefreshMs) {
    lastRefreshMs_ = nowMs;
    draw();
  }
}

void DateTimeScreen::draw() {
  const Theme theme = currentTheme(settings_);
  M5.Display.fillScreen(theme.background);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(theme.foreground, theme.background);
  M5.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5.Display.drawString(page_ == Page::Settings ? "Date & Time" : "RTC info",
                        M5.Display.width() / 2, 50);

  for (size_t i = 0; i < itemCount(); ++i) {
    drawRow(i, i == selected_);
  }

  drawFooter();
}

void DateTimeScreen::previewTouch(int32_t x, int32_t y) {
  const int row = rowAt(x, y);
  if (row < 0) return;
  selectRow(static_cast<size_t>(row));
}

void DateTimeScreen::handleTouch(int32_t x, int32_t y) {
  const int row = rowAt(x, y);
  if (row < 0) return;
  selectRow(static_cast<size_t>(row));
  activateSelected();
}

void DateTimeScreen::onButtonA() {
  selectRow((selected_ + 1) % itemCount());
}

void DateTimeScreen::onButtonB() {
  activateSelected();
}

void DateTimeScreen::activateSelected() {
  if (page_ == Page::RtcInfo) {
    if (selected_ == 0 || selected_ == itemCount() - 1) {
      page_ = Page::Settings;
      selected_ = 0;
      draw();
    }
    return;
  }

  switch (selected_) {
    case 0:
      cycleCountry();
      break;
    case 1:
      cycleTimeZone();
      break;
    case 2:
      cycleDateFormat();
      break;
    case 3:
      cycleTimeFormat();
      break;
    case 4:
      settings_.setAutomaticTimeEnabled(!settings_.automaticTimeEnabled());
      status_ = settings_.automaticTimeEnabled() ? "Auto time on" : "Manual enabled";
      break;
    case 5:
      status_ = timeService_.syncNow(millis()) ? "Sync successful" : "Sync pending";
      break;
    case 6:
      status_ = timeService_.adjustManualMinutes(1) ? "+1 minute" : "Turn auto off";
      break;
    case 7:
      status_ = timeService_.adjustManualMinutes(-1) ? "-1 minute" : "Turn auto off";
      break;
    case 8:
      page_ = Page::RtcInfo;
      selected_ = 0;
      lastRefreshMs_ = 0;
      draw();
      return;
    default:
      goBack();
      return;
  }

  timeService_.applyConfiguredTimezone();
  draw();
}

void DateTimeScreen::selectRow(size_t index) {
  if (index >= itemCount() || index == selected_) return;
  const size_t previous = selected_;
  selected_ = index;
  drawRow(previous, false);
  drawRow(selected_, true);
  drawFooter();
}

void DateTimeScreen::drawRow(size_t index, bool selected) {
  if (index >= itemCount()) return;
  const Theme theme = currentTheme(settings_);
  const int y = kRowStartY + static_cast<int>(index) * kRowHeight;
  const uint16_t fill = selected ? theme.selected : theme.background;
  const uint16_t border = selected ? theme.foreground : theme.panel;
  const String label(rowLabel(index));
  const String value = rowValue(index);

  M5.Display.fillRoundRect(kRowLeft, y, kRowWidth, kRowRectHeight, 10, fill);
  M5.Display.drawRoundRect(kRowLeft, y, kRowWidth, kRowRectHeight, 10, border);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextDatum(middle_left);
  M5.Display.setTextColor(theme.foreground, fill);
  M5.Display.drawString(fitTextToWidth(label, 178), kRowLeft + 16, y + (kRowRectHeight / 2));
  M5.Display.setTextDatum(middle_right);
  M5.Display.setTextColor(theme.muted, fill);
  M5.Display.drawString(fitTextToWidth(value, 184), kRowLeft + kRowWidth - 16,
                        y + (kRowRectHeight / 2));
}

void DateTimeScreen::drawFooter() {
  const Theme theme = currentTheme(settings_);
  M5.Display.fillRect(18, 416, 430, 40, theme.background);
  M5.Display.setFont(&fonts::FreeSans9pt7b);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(theme.muted, theme.background);
  const String status = status_.length() > 0 ? status_ : String(timeZoneIanaName(settings_.timeZone()));
  M5.Display.drawString(fitTextToWidth(status, 360), M5.Display.width() / 2, 428);
  M5.Display.drawString("A: Next     B: Change", M5.Display.width() / 2, 448);
}

int DateTimeScreen::rowAt(int32_t x, int32_t y) const {
  if (x < kRowLeft || x > kRowLeft + kRowWidth) return -1;
  for (size_t i = 0; i < itemCount(); ++i) {
    const int rowY = kRowStartY + static_cast<int>(i) * kRowHeight;
    if (y >= rowY - 3 && y <= rowY + kRowRectHeight + 3) return static_cast<int>(i);
  }
  return -1;
}

size_t DateTimeScreen::itemCount() const {
  return page_ == Page::Settings ? kSettingsItemCount : kRtcInfoItemCount;
}

const char* DateTimeScreen::rowLabel(size_t index) const {
  if (page_ == Page::RtcInfo) {
    constexpr const char* labels[] = {
        "Back", "RTC", "RTC time", "System time", "Time zone",
        "UTC offset", "DST", "Last NTP", "Back"};
    return labels[index];
  }

  constexpr const char* labels[] = {
      "Country", "Time zone", "Date format", "Time format", "Automatic",
      "Sync now", "Manual +1m", "Manual -1m", "RTC info", "Back"};
  return labels[index];
}

String DateTimeScreen::rowValue(size_t index) const {
  if (page_ == Page::RtcInfo) {
    const DateTimeSnapshot now = timeService_.now();
    switch (index) {
      case 0:
      case 8:
        return "";
      case 1:
        return timeService_.rtcAvailable() ? "Available" : "Unavailable";
      case 2:
      case 3:
        return timeService_.formatDateTime(now);
      case 4:
        return timeZoneIanaName(settings_.timeZone());
      case 5:
        return timeService_.utcOffsetText();
      case 6:
        return timeService_.dstText();
      case 7:
        return timeService_.lastNtpSyncText();
      default:
        return "";
    }
  }

  switch (index) {
    case 0:
      return countryRegionName(settings_.countryRegion());
    case 1:
      return timeZoneName(settings_.timeZone());
    case 2:
      return dateFormatName(settings_.dateFormat());
    case 3:
      return timeFormatName(settings_.timeFormat());
    case 4:
      return settings_.automaticTimeEnabled() ? "On" : "Off";
    case 5:
      return timeService_.ntpSynchronized() ? "Synced" : "Run";
    case 6:
    case 7:
      return settings_.automaticTimeEnabled() ? "Auto on" : "Ready";
    case 8:
      return "Open";
    default:
      return "";
  }
}

void DateTimeScreen::cycleCountry() {
  const uint8_t next = (static_cast<uint8_t>(settings_.countryRegion()) + 1) %
                       (static_cast<uint8_t>(CountryRegion::Europe) + 1);
  settings_.setCountryRegion(static_cast<CountryRegion>(next));
  status_ = String(localeCode(settings_.countryRegion())) + " defaults";
}

void DateTimeScreen::cycleDateFormat() {
  const uint8_t next = (static_cast<uint8_t>(settings_.dateFormat()) + 1) %
                       (static_cast<uint8_t>(DateFormat::DayMonthName) + 1);
  settings_.setDateFormat(static_cast<DateFormat>(next));
  status_ = "Date format saved";
}

void DateTimeScreen::cycleTimeFormat() {
  const uint8_t next = (static_cast<uint8_t>(settings_.timeFormat()) + 1) %
                       (static_cast<uint8_t>(TimeFormat::TwentyFourHour) + 1);
  settings_.setTimeFormat(static_cast<TimeFormat>(next));
  status_ = "Time format saved";
}

void DateTimeScreen::cycleTimeZone() {
  const uint8_t next = (static_cast<uint8_t>(settings_.timeZone()) + 1) %
                       (static_cast<uint8_t>(TimeZoneId::Utc) + 1);
  settings_.setTimeZone(static_cast<TimeZoneId>(next));
  status_ = timeZoneIanaName(settings_.timeZone());
}

void DateTimeScreen::goBack() {
  if (manager_) manager_->show(ScreenId::Settings);
}

}  // namespace iris
