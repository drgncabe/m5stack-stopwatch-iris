#include "iris/screens/ScreenManager.h"

namespace iris {

ScreenManager::ScreenManager() {
  screens_.fill(nullptr);
}

void ScreenManager::registerScreen(ScreenId id, Screen* screen) {
  if (!screen) return;
  const auto index = static_cast<size_t>(id);
  if (index >= screens_.size()) return;
  screens_[index] = screen;
  screen->attach(this);
}

void ScreenManager::show(ScreenId id) {
  const auto index = static_cast<size_t>(id);
  if (index >= screens_.size() || screens_[index] == nullptr) return;

  currentId_ = id;
  current_ = screens_[index];
  current_->enter();
  current_->draw();
}

void ScreenManager::redraw() {
  if (!current_) return;
  current_->enter();
  current_->draw();
}

void ScreenManager::update(uint32_t nowMs) {
  if (current_) current_->update(nowMs);
}

void ScreenManager::previewTouch(int32_t x, int32_t y) {
  if (current_) current_->previewTouch(x, y);
}

void ScreenManager::handleTouch(int32_t x, int32_t y) {
  if (current_) current_->handleTouch(x, y);
}

void ScreenManager::onButtonA() {
  if (current_) current_->onButtonA();
}

void ScreenManager::onButtonB() {
  if (current_) current_->onButtonB();
}

}  // namespace iris
