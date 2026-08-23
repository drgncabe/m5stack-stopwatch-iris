#pragma once

#include <Arduino.h>

namespace iris {

enum class EventType : uint8_t {
  AppStarted,
  AppPaused,
  AppResumed,
  AppStopped,
  WifiConnected,
  WifiDisconnected,
  BatteryLow,
  ImuOrientationChanged,
};

struct Event {
  EventType type;
  uint32_t timestampMs;
  const char* source;
  const char* text;
  int32_t value;
  void* data;
};

using EventHandler = void (*)(void* context, const Event& event);

class EventBus {
 public:
  bool subscribe(EventType type, EventHandler handler, void* context = nullptr);
  void unsubscribe(EventType type, EventHandler handler, void* context = nullptr);

  void publish(EventType type,
               const char* source = nullptr,
               const char* text = nullptr,
               int32_t value = 0,
               void* data = nullptr);
  void publish(const Event& event);

  size_t subscriberCount() const;
  uint32_t publishedCount() const { return publishedCount_; }
  EventType lastEventType() const { return lastEventType_; }
  const char* lastEventName() const;
  const char* eventName(EventType type) const;
  String summary() const;

 private:
  struct Subscription {
    EventType type = EventType::AppStarted;
    EventHandler handler = nullptr;
    void* context = nullptr;
  };

  static constexpr size_t kMaxSubscriptions = 24;

  Subscription subscriptions_[kMaxSubscriptions]{};
  uint32_t publishedCount_ = 0;
  EventType lastEventType_ = EventType::AppStarted;
};

}  // namespace iris
