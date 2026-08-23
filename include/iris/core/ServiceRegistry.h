#pragma once

#include <Arduino.h>

namespace iris {

struct ServiceDescriptor {
  const char* id;
  const char* name;
  bool started;
};

class ServiceRegistry {
 public:
  bool registerService(const char* id, const char* name, bool started = true);
  void setStarted(const char* id, bool started);

  const ServiceDescriptor* service(size_t index) const;
  const ServiceDescriptor* find(const char* id) const;
  size_t count() const { return count_; }
  size_t startedCount() const;
  String summary() const;

 private:
  static constexpr size_t kMaxServices = 14;

  ServiceDescriptor services_[kMaxServices]{};
  size_t count_ = 0;
};

}  // namespace iris
