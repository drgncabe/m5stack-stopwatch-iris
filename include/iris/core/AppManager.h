#pragma once

#include <Arduino.h>

#include "iris/core/EventBus.h"
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
  virtual bool onButtonA() { return false; }
  virtual bool onButtonB() { return false; }
  virtual bool onTouch(int32_t, int32_t) { return false; }
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

  void setEventBus(EventBus* events);
  bool registerApp(const AppDescriptor& app);
  bool begin();
  bool launch(const char* id);
  bool switchTo(ScreenId screen);
  bool syncToCurrentScreen();
  bool stopCurrent();
  void update(uint32_t nowMs);
  void render();
  bool onButtonA();
  bool onButtonB();
  bool onTouch(int32_t x, int32_t y);

  const AppDescriptor* current() const;
  AppLifecycleState currentState() const;
  const char* currentStateName() const;
  const AppDescriptor* findById(const char* id) const;
  const AppDescriptor* findByScreen(ScreenId screen) const;
  const AppDescriptor* appAt(size_t index) const;
  AppLifecycleState stateAt(size_t index) const;
  size_t count() const { return count_; }

 private:
  static constexpr size_t kMaxApps = 20;
  static constexpr size_t kNoApp = kMaxApps;

  bool activateIndex(size_t index, bool showScreen);
  size_t findIndexById(const char* id) const;
  size_t findIndexByScreen(ScreenId screen) const;
  bool ensureAppBegun(size_t index);
  void publishLifecycle(EventType type, const AppDescriptor& app);

  ScreenManager& screens_;
  EventBus* events_ = nullptr;
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
