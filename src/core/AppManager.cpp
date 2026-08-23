#include "iris/core/AppManager.h"

#include <string.h>

namespace iris {

AppManager::AppManager(ScreenManager& screens) : screens_(screens) {}

bool AppManager::registerApp(const AppDescriptor& app) {
  if (!app.id || !app.name || count_ >= kMaxApps || findById(app.id)) return false;
  apps_[count_] = app;
  count_++;
  return true;
}

bool AppManager::launch(const char* id) {
  const AppDescriptor* app = findById(id);
  if (!app) return false;
  current_ = app;
  screens_.show(app->screen);
  return true;
}

bool AppManager::switchTo(ScreenId screen) {
  const AppDescriptor* app = findByScreen(screen);
  if (app) {
    current_ = app;
  }
  screens_.show(screen);
  return app != nullptr;
}

const AppDescriptor* AppManager::current() const {
  const AppDescriptor* screenApp = findByScreen(screens_.currentId());
  return screenApp ? screenApp : current_;
}

const AppDescriptor* AppManager::findById(const char* id) const {
  if (!id) return nullptr;
  for (size_t i = 0; i < count_; ++i) {
    if (strcmp(apps_[i].id, id) == 0) return &apps_[i];
  }
  return nullptr;
}

const AppDescriptor* AppManager::findByScreen(ScreenId screen) const {
  for (size_t i = 0; i < count_; ++i) {
    if (apps_[i].screen == screen) return &apps_[i];
  }
  return nullptr;
}

const char* appKindName(AppKind kind) {
  switch (kind) {
    case AppKind::Settings: return "Settings";
    case AppKind::Developer: return "Developer";
    case AppKind::Fidget: return "Fidget";
    default: return "System";
  }
}

}  // namespace iris
