#pragma once

#include <Arduino.h>

#include "iris/screens/ScreenManager.h"

namespace iris {

enum class AppKind : uint8_t {
  System,
  Settings,
  Developer,
  Fidget,
};

struct AppDescriptor {
  const char* id;
  const char* name;
  ScreenId screen;
  AppKind kind;
  bool visible;
};

class AppManager {
 public:
  explicit AppManager(ScreenManager& screens);

  bool registerApp(const AppDescriptor& app);
  bool launch(const char* id);
  bool switchTo(ScreenId screen);

  const AppDescriptor* current() const;
  const AppDescriptor* findById(const char* id) const;
  const AppDescriptor* findByScreen(ScreenId screen) const;
  size_t count() const { return count_; }

 private:
  static constexpr size_t kMaxApps = 20;

  ScreenManager& screens_;
  AppDescriptor apps_[kMaxApps]{};
  size_t count_ = 0;
  const AppDescriptor* current_ = nullptr;
};

const char* appKindName(AppKind kind);

}  // namespace iris
