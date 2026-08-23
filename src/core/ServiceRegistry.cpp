#include "iris/core/ServiceRegistry.h"

#include <string.h>

namespace iris {

bool ServiceRegistry::registerService(const char* id, const char* name, bool started) {
  if (!id || !name || count_ >= kMaxServices || find(id)) return false;
  services_[count_] = {id, name, started};
  count_++;
  return true;
}

void ServiceRegistry::setStarted(const char* id, bool started) {
  ServiceDescriptor* existing = const_cast<ServiceDescriptor*>(find(id));
  if (existing) existing->started = started;
}

const ServiceDescriptor* ServiceRegistry::service(size_t index) const {
  if (index >= count_) return nullptr;
  return &services_[index];
}

const ServiceDescriptor* ServiceRegistry::find(const char* id) const {
  if (!id) return nullptr;
  for (size_t i = 0; i < count_; ++i) {
    if (strcmp(services_[i].id, id) == 0) return &services_[i];
  }
  return nullptr;
}

size_t ServiceRegistry::startedCount() const {
  size_t total = 0;
  for (size_t i = 0; i < count_; ++i) {
    if (services_[i].started) total++;
  }
  return total;
}

String ServiceRegistry::summary() const {
  String text;
  text.reserve(96);
  text += String(startedCount());
  text += "/";
  text += String(count_);
  text += " started";
  return text;
}

}  // namespace iris
