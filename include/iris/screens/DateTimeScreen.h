#pragma once

#include "iris/screens/Screen.h"
#include "iris/services/SettingsStore.h"
#include "iris/services/TimeService.h"

namespace iris {

class DateTimeScreen : public Screen {
 public:
  DateTimeScreen(SettingsStore& settings, TimeService& timeService)
      : settings_(settings), timeService_(timeService) {}

  void enter() override;
  void update(uint32_t nowMs) override;
  void draw() override;
  void previewTouch(int32_t x, int32_t y) override;
  void handleTouch(int32_t x, int32_t y) override;
  void onButtonA() override;
  void onButtonB() override;

 private:
  enum class Page : uint8_t {
    Settings = 0,
    RtcInfo = 1,
  };

  void activateSelected();
  void selectRow(size_t index);
  void drawRow(size_t index, bool selected);
  void drawFooter();
  int rowAt(int32_t x, int32_t y) const;
  size_t itemCount() const;
  const char* rowLabel(size_t index) const;
  String rowValue(size_t index) const;
  void cycleCountry();
  void cycleDateFormat();
  void cycleTimeFormat();
  void cycleTimeZone();
  void goBack();

  SettingsStore& settings_;
  TimeService& timeService_;
  Page page_ = Page::Settings;
  size_t selected_ = 0;
  uint32_t lastRefreshMs_ = 0;
  String status_;
};

}  // namespace iris
