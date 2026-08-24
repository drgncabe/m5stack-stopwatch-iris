#include "iris/core/EventBus.h"

namespace iris {

bool EventBus::subscribe(EventType type, EventHandler handler, void* context) {
  if (!handler) return false;

  for (size_t i = 0; i < kMaxSubscriptions; ++i) {
    const Subscription& subscription = subscriptions_[i];
    if (subscription.handler == handler && subscription.context == context &&
        subscription.type == type) {
      return true;
    }
  }

  for (size_t i = 0; i < kMaxSubscriptions; ++i) {
    if (!subscriptions_[i].handler) {
      subscriptions_[i].type = type;
      subscriptions_[i].handler = handler;
      subscriptions_[i].context = context;
      return true;
    }
  }

  return false;
}

void EventBus::unsubscribe(EventType type, EventHandler handler, void* context) {
  if (!handler) return;

  for (size_t i = 0; i < kMaxSubscriptions; ++i) {
    Subscription& subscription = subscriptions_[i];
    if (subscription.handler == handler && subscription.context == context &&
        subscription.type == type) {
      subscription.handler = nullptr;
      subscription.context = nullptr;
      return;
    }
  }
}

void EventBus::publish(EventType type,
                       const char* source,
                       const char* text,
                       int32_t value,
                       void* data) {
  Event event{type, millis(), source, text, value, data};
  publish(event);
}

void EventBus::publish(const Event& event) {
  publishedCount_++;
  lastEventType_ = event.type;

  for (size_t i = 0; i < kMaxSubscriptions; ++i) {
    const Subscription& subscription = subscriptions_[i];
    if (subscription.handler && subscription.type == event.type) {
      subscription.handler(subscription.context, event);
    }
  }
}

size_t EventBus::subscriberCount() const {
  size_t total = 0;
  for (size_t i = 0; i < kMaxSubscriptions; ++i) {
    if (subscriptions_[i].handler) total++;
  }
  return total;
}

const char* EventBus::lastEventName() const {
  return eventName(lastEventType_);
}

const char* EventBus::eventName(EventType type) const {
  switch (type) {
    case EventType::AppStarted: return "APP_STARTED";
    case EventType::AppPaused: return "APP_PAUSED";
    case EventType::AppResumed: return "APP_RESUMED";
    case EventType::AppStopped: return "APP_STOPPED";
    case EventType::WifiConnected: return "WIFI_CONNECTED";
    case EventType::WifiDisconnected: return "WIFI_DISCONNECTED";
    case EventType::BatteryLow: return "BATTERY_LOW";
    case EventType::ImuOrientationChanged: return "IMU_ORIENTATION_CHANGED";
    case EventType::TimeSynchronized: return "TIME_SYNCHRONIZED";
    case EventType::TimeSyncFailed: return "TIME_SYNC_FAILED";
    case EventType::TimeChanged: return "TIME_CHANGED";
    case EventType::TimeZoneChanged: return "TIMEZONE_CHANGED";
    case EventType::LocaleChanged: return "LOCALE_CHANGED";
    case EventType::RtcTimeInvalid: return "RTC_TIME_INVALID";
  }
  return "UNKNOWN";
}

String EventBus::summary() const {
  String text;
  text.reserve(72);
  text += String(publishedCount_);
  text += " published, ";
  text += String(subscriberCount());
  text += " subscribers, last ";
  text += lastEventName();
  return text;
}

}  // namespace iris
