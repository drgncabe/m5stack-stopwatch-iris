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

enum class AppLifecycleState : uint8_t {
  Registered,
  Started,
  Paused,
  Stopped,
};

class IrisApplication {
 public:
  virtual ~IrisApplication() = default;

  virtual const char* id() const = 0;
  virtual const char* name() const = 0;
  virtual bool begin() { return true; }
  virtual void onStart() {}
  virtual void onPause() {}
  virtual void onResume() {}
  virtual void onStop() {}
  virtual void update(uint32_t) {}
  virtual void render() {}
  virtual void onButtonA() {}
  virtual void onButtonB() {}
  virtual void onTouch(int32_t, int32_t) {}
};

struct AppDescriptor {
  constexpr AppDescriptor()
      : id(nullptr),
        name(nullptr),
        screen(ScreenId::Watch),
        kind(AppKind::System),
        visible(false),
        application(nullptr) {}

  constexpr AppDescriptor(const char* appId,
                          const char* appName,
                          ScreenId appScreen,
                          AppKind appKind,
                          bool appVisible,
                          IrisApplication* appInstance = nullptr)
      : id(appId),
        name(appName),
        screen(appScreen),
        kind(appKind),
        visible(appVisible),
        application(appInstance) {}

  const char* id;
  const char* name;
  ScreenId screen;
  AppKind kind;
  bool visible;
  IrisApplication* application;
};

class AppManager {
 public:
  explicit AppManager(ScreenManager& screens);

  bool registerApp(const AppDescriptor& app);
  bool begin();
  bool launch(const char* id);
  bool switchTo(ScreenId screen);
  bool syncToCurrentScreen();
  bool stopCurrent();
  void update(uint32_t nowMs);
  void render();
  void onButtonA();
  void onButtonB();
  void onTouch(int32_t x, int32_t y);

  const AppDescriptor* current() const;
  AppLifecycleState currentState() const;
  const char* currentStateName() const;
  const AppDescriptor* findById(const char* id) const;
  const AppDescriptor* findByScreen(ScreenId screen) const;
  size_t count() const { return count_; }

 private:
  static constexpr size_t kMaxApps = 20;
  static constexpr size_t kNoApp = kMaxApps;

  bool activateIndex(size_t index, bool showScreen);
  size_t findIndexById(const char* id) const;
  size_t findIndexByScreen(ScreenId screen) const;
  bool ensureAppBegun(size_t index);

  ScreenManager& screens_;
  AppDescriptor apps_[kMaxApps]{};
  AppLifecycleState states_[kMaxApps]{};
  bool begun_[kMaxApps]{};
  size_t count_ = 0;
  size_t currentIndex_ = kNoApp;
  const AppDescriptor* current_ = nullptr;
};

const char* appKindName(AppKind kind);
const char* appLifecycleStateName(AppLifecycleState state);

}  // namespace iris
