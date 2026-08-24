#include "iris/core/AppManager.h"

#include <string.h>

namespace iris {

AppManager::AppManager(ScreenManager& screens) : screens_(screens) {}

void AppManager::setEventBus(EventBus* events) {
  events_ = events;
}

bool AppManager::registerApp(const AppDescriptor& app) {
  if (!app.id || !app.name || count_ >= kMaxApps || findById(app.id)) return false;
  apps_[count_] = app;
  states_[count_] = AppLifecycleState::Registered;
  begun_[count_] = false;
  count_++;
  return true;
}

bool AppManager::begin() {
  bool ok = true;
  for (size_t i = 0; i < count_; ++i) {
    if (!ensureAppBegun(i)) ok = false;
  }
  return ok;
}

bool AppManager::launch(const char* id) {
  const size_t index = findIndexById(id);
  if (index == kNoApp) return false;
  return activateIndex(index, true);
}

bool AppManager::switchTo(ScreenId screen) {
  const size_t index = findIndexByScreen(screen);
  if (index == kNoApp) {
    screens_.show(screen);
    return false;
  }
  return activateIndex(index, true);
}

bool AppManager::syncToCurrentScreen() {
  const size_t index = findIndexByScreen(screens_.currentId());
  if (index == kNoApp || index == currentIndex_) return index != kNoApp;
  return activateIndex(index, false);
}

bool AppManager::stopCurrent() {
  if (currentIndex_ == kNoApp) return false;
  IrisApplication* app = apps_[currentIndex_].application;
  if (states_[currentIndex_] != AppLifecycleState::Stopped) {
    if (app) app->onStop();
    publishLifecycle(EventType::AppStopped, apps_[currentIndex_]);
  }
  states_[currentIndex_] = AppLifecycleState::Stopped;
  currentIndex_ = kNoApp;
  current_ = nullptr;
  return true;
}

void AppManager::update(uint32_t nowMs) {
  if (currentIndex_ == kNoApp) return;
  IrisApplication* app = apps_[currentIndex_].application;
  if (app) app->update(nowMs);
}

void AppManager::render() {
  if (currentIndex_ == kNoApp) return;
  IrisApplication* app = apps_[currentIndex_].application;
  if (app) app->render();
}

bool AppManager::onButtonA() {
  if (currentIndex_ == kNoApp) return false;
  IrisApplication* app = apps_[currentIndex_].application;
  if (!app) return false;
  return app->onButtonA();
}

bool AppManager::onButtonB() {
  if (currentIndex_ == kNoApp) return false;
  IrisApplication* app = apps_[currentIndex_].application;
  if (!app) return false;
  return app->onButtonB();
}

bool AppManager::onTouch(int32_t x, int32_t y) {
  if (currentIndex_ == kNoApp) return false;
  IrisApplication* app = apps_[currentIndex_].application;
  if (!app) return false;
  return app->onTouch(x, y);
}

const AppDescriptor* AppManager::current() const {
  const AppDescriptor* screenApp = findByScreen(screens_.currentId());
  return screenApp ? screenApp : current_;
}

AppLifecycleState AppManager::currentState() const {
  if (currentIndex_ == kNoApp) return AppLifecycleState::Stopped;
  return states_[currentIndex_];
}

const char* AppManager::currentStateName() const {
  return appLifecycleStateName(currentState());
}

const AppDescriptor* AppManager::findById(const char* id) const {
  const size_t index = findIndexById(id);
  return index == kNoApp ? nullptr : &apps_[index];
}

const AppDescriptor* AppManager::findByScreen(ScreenId screen) const {
  const size_t index = findIndexByScreen(screen);
  return index == kNoApp ? nullptr : &apps_[index];
}

const AppDescriptor* AppManager::appAt(size_t index) const {
  return index < count_ ? &apps_[index] : nullptr;
}

AppLifecycleState AppManager::stateAt(size_t index) const {
  return index < count_ ? states_[index] : AppLifecycleState::Stopped;
}

bool AppManager::activateIndex(size_t index, bool showScreen) {
  if (index >= count_) return false;

  if (currentIndex_ != kNoApp && currentIndex_ != index &&
      states_[currentIndex_] == AppLifecycleState::Started) {
    if (apps_[currentIndex_].application) apps_[currentIndex_].application->onPause();
    states_[currentIndex_] = AppLifecycleState::Paused;
    publishLifecycle(EventType::AppPaused, apps_[currentIndex_]);
  }

  currentIndex_ = index;
  current_ = &apps_[index];

  if (!ensureAppBegun(index)) return false;

  IrisApplication* app = apps_[index].application;
  if (states_[index] == AppLifecycleState::Paused) {
    if (app) app->onResume();
    publishLifecycle(EventType::AppResumed, apps_[index]);
  } else if (states_[index] != AppLifecycleState::Started) {
    if (app) app->onStart();
    publishLifecycle(EventType::AppStarted, apps_[index]);
  }
  states_[index] = AppLifecycleState::Started;

  if (showScreen) screens_.show(apps_[index].screen);
  return true;
}

size_t AppManager::findIndexById(const char* id) const {
  if (!id) return kNoApp;
  for (size_t i = 0; i < count_; ++i) {
    if (strcmp(apps_[i].id, id) == 0) return i;
  }
  return kNoApp;
}

size_t AppManager::findIndexByScreen(ScreenId screen) const {
  for (size_t i = 0; i < count_; ++i) {
    if (apps_[i].screen == screen) return i;
  }
  return kNoApp;
}

bool AppManager::ensureAppBegun(size_t index) {
  if (index >= count_ || begun_[index]) return index < count_;
  IrisApplication* app = apps_[index].application;
  if (app && !app->begin()) {
    states_[index] = AppLifecycleState::Stopped;
    return false;
  }
  begun_[index] = true;
  return true;
}

void AppManager::publishLifecycle(EventType type, const AppDescriptor& app) {
  if (!events_) return;
  events_->publish(type, "AppManager", app.id);
}

const char* appKindName(AppKind kind) {
  switch (kind) {
    case AppKind::Settings: return "Settings";
    case AppKind::Developer: return "Developer";
    case AppKind::Fidget: return "Fidget";
    default: return "System";
  }
}

const char* appLifecycleStateName(AppLifecycleState state) {
  switch (state) {
    case AppLifecycleState::Registered: return "Registered";
    case AppLifecycleState::Started: return "Started";
    case AppLifecycleState::Paused: return "Paused";
    case AppLifecycleState::Stopped: return "Stopped";
  }
  return "Unknown";
}

const char* appUpdateClassName(AppUpdateClass updateClass) {
  switch (updateClass) {
    case AppUpdateClass::Interactive: return "Interactive";
    case AppUpdateClass::Realtime: return "Realtime";
    case AppUpdateClass::Background: return "Background";
    default: return "Normal";
  }
}

}  // namespace iris
