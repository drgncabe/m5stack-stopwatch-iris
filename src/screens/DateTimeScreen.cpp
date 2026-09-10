#include "iris/screens/DateTimeScreen.h"

#include <M5Unified.h>
#include <time.h>

#include "iris/Theme.h"
#include "iris/screens/ScreenManager.h"
#include "iris/screens/SettingsListRenderer.h"

namespace iris {

namespace {
constexpr size_t kSettingsItemCount = 9;
constexpr size_t kManualItemCount = 8;
constexpr size_t kRtcInfoItemCount = 10;
constexpr uint32_t kRefreshMs = 1000;

constexpr SettingsListLayout kListLayout = SettingsListRenderer::defaultLayout();
}  // namespace

void DateTimeScreen::enter() {
  page_ = Page::Settings;
  selected_ = 0;
  lastRefreshMs_ = 0;
  status_ = "";
  pendingSyncRefresh_ = false;
}

void DateTimeScreen::update(uint32_t nowMs) {
  if (page_ != Page::RtcInfo && page_ != Page::Manual &&
      !(page_ == Page::Settings && selected_ == 5 &&
        (timeService_.syncInProgress() || pendingSyncRefresh_))) {
    return;
  }
  if (lastRefreshMs_ == 0 || nowMs - lastRefreshMs_ >= kRefreshMs) {
    lastRefreshMs_ = nowMs;
    if (page_ == Page::Settings && selected_ == 5) {
      status_ = timeService_.syncStatusText();
      if (!timeService_.syncInProgress()) pendingSyncRefresh_ = false;
    }
    draw();
  }
}

void DateTimeScreen::draw() {
  const Theme theme = currentTheme(settings_);
  M5.Display.fillScreen(theme.background);
  const char* title = "Date & Time";
  if (page_ == Page::Manual) title = "Manual time";
  if (page_ == Page::RtcInfo) title = "RTC info";
  SettingsListRenderer::drawTitle(theme, title);

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
  if (page_ == Page::Manual) {
    switch (selected_) {
      case 0:
      case 7:
        page_ = Page::Settings;
        selected_ = 0;
        draw();
        return;
      case 1:
        cycleManualField();
        break;
      case 2:
        adjustManualField(1);
        break;
      case 3:
        adjustManualField(-1);
        break;
      case 4:
        applyManualDraft();
        break;
      default:
        break;
    }
    draw();
    return;
  }

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
      timeService_.syncNow(millis());
      status_ = timeService_.syncStatusText();
      lastRefreshMs_ = 0;
      pendingSyncRefresh_ = true;
      break;
    case 6:
      openManualEditor();
      return;
    case 7:
      page_ = Page::RtcInfo;
      selected_ = 0;
      lastRefreshMs_ = 0;
      draw();
      return;
    default:
      goBack();
      break;
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
  const String label(rowLabel(index));
  const String value = rowValue(index);
  SettingsListRenderer::drawRow(currentTheme(settings_), kListLayout, index, label, value, selected);
}

void DateTimeScreen::drawFooter() {
  const Theme theme = currentTheme(settings_);
  const String status = status_.length() > 0 ? status_ : String(timeZoneIanaName(settings_.timeZone()));
  SettingsListRenderer::drawFooter(theme, status, "A: Next     B: Change");
}

int DateTimeScreen::rowAt(int32_t x, int32_t y) const {
  return SettingsListRenderer::rowAt(kListLayout, itemCount(), x, y);
}

size_t DateTimeScreen::itemCount() const {
  if (page_ == Page::Manual) return kManualItemCount;
  return page_ == Page::Settings ? kSettingsItemCount : kRtcInfoItemCount;
}

const char* DateTimeScreen::rowLabel(size_t index) const {
  if (page_ == Page::Manual) {
    constexpr const char* labels[] = {
        "Back", "Field", "Increase", "Decrease", "Apply", "Date", "Time", "Back"};
    return labels[index];
  }

  if (page_ == Page::RtcInfo) {
    constexpr const char* labels[] = {
        "Back", "RTC", "RTC time", "System time", "Time zone",
        "UTC offset", "DST", "Last NTP", "Clock diff", "Back"};
    return labels[index];
  }

  constexpr const char* labels[] = {
      "Country", "Time zone", "Date format", "Time format", "Automatic",
      "Sync now", "Manual time", "RTC info", "Back"};
  return labels[index];
}

String DateTimeScreen::rowValue(size_t index) const {
  if (page_ == Page::Manual) {
    switch (index) {
      case 0:
      case 7:
        return "";
      case 1:
        return manualFieldName();
      case 2:
      case 3:
        return settings_.automaticTimeEnabled() ? "Auto on" : manualFieldValue();
      case 4:
        return settings_.automaticTimeEnabled() ? "Disabled" : "Save";
      case 5:
        return timeService_.formatDate(manualDraft_);
      case 6:
        return timeService_.formatTime(manualDraft_);
      default:
        return "";
    }
  }

  if (page_ == Page::RtcInfo) {
    switch (index) {
      case 0:
      case 9:
        return "";
      case 1:
        return timeService_.rtcAvailable() ? "Available" : "Unavailable";
      case 2:
        return timeService_.formatDateTime(timeService_.rtcNow());
      case 3:
        return timeService_.formatDateTime(timeService_.systemNow());
      case 4:
        return timeZoneIanaName(settings_.timeZone());
      case 5:
        return timeService_.utcOffsetText();
      case 6:
        return timeService_.dstText();
      case 7:
        return timeService_.lastNtpSyncText();
      case 8:
        return timeService_.rtcSystemDifferenceText();
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
      return timeService_.syncStatusText();
    case 6:
    case 7:
      return "Open";
    default:
      return "";
  }
}

void DateTimeScreen::openManualEditor() {
  manualDraft_ = timeService_.now();
  if (!manualDraft_.valid) {
    manualDraft_.year = 2026;
    manualDraft_.month = 1;
    manualDraft_.day = 1;
    manualDraft_.hour = 12;
    manualDraft_.minute = 0;
    manualDraft_.second = 0;
    manualDraft_.weekDay = 4;
    manualDraft_.valid = true;
  }
  manualField_ = ManualField::Year;
  page_ = Page::Manual;
  selected_ = 1;
  status_ = settings_.automaticTimeEnabled() ? "Turn auto off first" : "Edit then apply";
  draw();
}

void DateTimeScreen::cycleManualField() {
  const uint8_t next = (static_cast<uint8_t>(manualField_) + 1) %
                       (static_cast<uint8_t>(ManualField::Minute) + 1);
  manualField_ = static_cast<ManualField>(next);
  status_ = manualFieldName();
}

void DateTimeScreen::adjustManualField(int delta) {
  if (settings_.automaticTimeEnabled()) {
    status_ = "Turn auto off first";
    return;
  }

  switch (manualField_) {
    case ManualField::Year:
      manualDraft_.year += delta;
      manualDraft_.year = constrain(manualDraft_.year, 2024, 2099);
      break;
    case ManualField::Month:
      manualDraft_.month += delta;
      break;
    case ManualField::Day:
      manualDraft_.day += delta;
      break;
    case ManualField::Hour:
      manualDraft_.hour += delta;
      break;
    case ManualField::Minute:
      manualDraft_.minute += delta;
      break;
  }

  normalizeManualDraft();
  status_ = String(manualFieldName()) + " " + manualFieldValue();
}

void DateTimeScreen::normalizeManualDraft() {
  struct tm localTime {};
  localTime.tm_year = manualDraft_.year - 1900;
  localTime.tm_mon = manualDraft_.month - 1;
  localTime.tm_mday = manualDraft_.day;
  localTime.tm_hour = manualDraft_.hour;
  localTime.tm_min = manualDraft_.minute;
  localTime.tm_sec = manualDraft_.second;
  localTime.tm_isdst = -1;

  const time_t epoch = mktime(&localTime);
  if (epoch <= 0) return;

  struct tm normalized {};
  localtime_r(&epoch, &normalized);
  manualDraft_.year = normalized.tm_year + 1900;
  manualDraft_.month = normalized.tm_mon + 1;
  manualDraft_.day = normalized.tm_mday;
  manualDraft_.hour = normalized.tm_hour;
  manualDraft_.minute = normalized.tm_min;
  manualDraft_.second = normalized.tm_sec;
  manualDraft_.weekDay = normalized.tm_wday;
  manualDraft_.valid = true;
}

void DateTimeScreen::applyManualDraft() {
  if (settings_.automaticTimeEnabled()) {
    status_ = "Turn auto off first";
    return;
  }
  normalizeManualDraft();
  status_ = timeService_.setManualDateTime(manualDraft_) ? "Manual time saved" : "Could not save";
}

const char* DateTimeScreen::manualFieldName() const {
  switch (manualField_) {
    case ManualField::Month: return "Month";
    case ManualField::Day: return "Day";
    case ManualField::Hour: return "Hour";
    case ManualField::Minute: return "Minute";
    default: return "Year";
  }
}

String DateTimeScreen::manualFieldValue() const {
  switch (manualField_) {
    case ManualField::Month:
      return String(manualDraft_.month);
    case ManualField::Day:
      return String(manualDraft_.day);
    case ManualField::Hour:
      return String(manualDraft_.hour);
    case ManualField::Minute:
      return String(manualDraft_.minute);
    default:
      return String(manualDraft_.year);
  }
}

void DateTimeScreen::cycleCountry() {
  const uint8_t next = (static_cast<uint8_t>(settings_.countryRegion()) + 1) %
                       kCountryRegionCount;
  settings_.setCountryRegion(static_cast<CountryRegion>(next));
  status_ = String(localeCode(settings_.countryRegion())) + " defaults";
  events_.publish(EventType::LocaleChanged, "DateTimeScreen", localeCode(settings_.countryRegion()));
}

void DateTimeScreen::cycleDateFormat() {
  const uint8_t next = (static_cast<uint8_t>(settings_.dateFormat()) + 1) %
                       kDateFormatCount;
  settings_.setDateFormat(static_cast<DateFormat>(next));
  status_ = "Date format saved";
}

void DateTimeScreen::cycleTimeFormat() {
  const uint8_t next = (static_cast<uint8_t>(settings_.timeFormat()) + 1) %
                       kTimeFormatCount;
  settings_.setTimeFormat(static_cast<TimeFormat>(next));
  status_ = "Time format saved";
}

void DateTimeScreen::cycleTimeZone() {
  const uint8_t next = (static_cast<uint8_t>(settings_.timeZone()) + 1) % kTimeZoneCount;
  settings_.setTimeZone(static_cast<TimeZoneId>(next));
  status_ = timeZoneIanaName(settings_.timeZone());
  events_.publish(EventType::TimeZoneChanged, "DateTimeScreen", timeZoneIanaName(settings_.timeZone()));
}

void DateTimeScreen::goBack() {
  if (manager_) manager_->show(ScreenId::Settings);
}

}  // namespace iris
