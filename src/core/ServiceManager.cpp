#include "iris/core/ServiceManager.h"

#include <string.h>

namespace iris {

bool ServiceManager::registerService(const char* id, const char* name, bool started) {
  return registerService(id, name, nullptr, started);
}

bool ServiceManager::registerService(const char* id, const char* name, void* instance, bool started) {
  if (!id || !name || count_ >= kMaxServices || find(id)) return false;
  services_[count_] = ServiceDescriptor(id, name, started, nullptr, instance);
  count_++;
  return true;
}

bool ServiceManager::registerManagedService(IrisService* service, bool startImmediately) {
  if (!service) return false;
  if (!service->id() || !service->name() || count_ >= kMaxServices || find(service->id())) return false;
  services_[count_] =
      ServiceDescriptor(service->id(), service->name(), startImmediately, service, service);
  services_[count_].state = ServiceLifecycleState::Registered;
  count_++;
  return true;
}

bool ServiceManager::begin() {
  bool ok = true;
  for (size_t i = 0; i < count_; ++i) {
    if (!beginDescriptor(services_[i])) ok = false;
  }
  return ok;
}

void ServiceManager::update(uint32_t nowMs) {
  for (size_t i = 0; i < count_; ++i) {
    ServiceDescriptor& descriptor = services_[i];
    if (descriptor.service && descriptor.state == ServiceLifecycleState::Started) {
      descriptor.service->update(nowMs);
    }
  }
}

void ServiceManager::suspend(const char* id) {
  ServiceDescriptor* descriptor = findMutable(id);
  if (!descriptor || descriptor->state != ServiceLifecycleState::Started) return;
  if (descriptor->service) descriptor->service->suspend();
  descriptor->state = ServiceLifecycleState::Suspended;
  descriptor->started = false;
}

void ServiceManager::resume(const char* id) {
  ServiceDescriptor* descriptor = findMutable(id);
  if (!descriptor || descriptor->state != ServiceLifecycleState::Suspended) return;
  if (descriptor->service) descriptor->service->resume();
  descriptor->state = ServiceLifecycleState::Started;
  descriptor->started = true;
}

void ServiceManager::stop(const char* id) {
  ServiceDescriptor* descriptor = findMutable(id);
  if (!descriptor || descriptor->state == ServiceLifecycleState::Stopped) return;
  if (descriptor->service) descriptor->service->stop();
  descriptor->state = ServiceLifecycleState::Stopped;
  descriptor->started = false;
}

void ServiceManager::setStarted(const char* id, bool started) {
  ServiceDescriptor* descriptor = findMutable(id);
  if (!descriptor) return;
  descriptor->started = started;
  descriptor->state = started ? ServiceLifecycleState::Started : ServiceLifecycleState::Stopped;
}

const ServiceDescriptor* ServiceManager::service(size_t index) const {
  if (index >= count_) return nullptr;
  return &services_[index];
}

const ServiceDescriptor* ServiceManager::find(const char* id) const {
  if (!id) return nullptr;
  for (size_t i = 0; i < count_; ++i) {
    if (strcmp(services_[i].id, id) == 0) return &services_[i];
  }
  return nullptr;
}

const char* ServiceManager::stateName(const char* id) const {
  const ServiceDescriptor* descriptor = find(id);
  return descriptor ? serviceLifecycleStateName(descriptor->state) : "Unknown";
}

size_t ServiceManager::startedCount() const {
  size_t total = 0;
  for (size_t i = 0; i < count_; ++i) {
    if (services_[i].started) total++;
  }
  return total;
}

String ServiceManager::summary() const {
  String text;
  text.reserve(96);
  text += String(startedCount());
  text += "/";
  text += String(count_);
  text += " started";
  return text;
}

ServiceDescriptor* ServiceManager::findMutable(const char* id) {
  return const_cast<ServiceDescriptor*>(find(id));
}

bool ServiceManager::beginDescriptor(ServiceDescriptor& descriptor) {
  if (descriptor.state != ServiceLifecycleState::Registered &&
      descriptor.state != ServiceLifecycleState::Stopped) {
    return descriptor.state != ServiceLifecycleState::Failed;
  }

  if (!descriptor.started) return true;
  if (descriptor.service && !descriptor.service->begin()) {
    descriptor.started = false;
    descriptor.state = ServiceLifecycleState::Failed;
    return false;
  }
  descriptor.state = ServiceLifecycleState::Started;
  return true;
}

const char* serviceLifecycleStateName(ServiceLifecycleState state) {
  switch (state) {
    case ServiceLifecycleState::Registered: return "Registered";
    case ServiceLifecycleState::Started: return "Started";
    case ServiceLifecycleState::Suspended: return "Suspended";
    case ServiceLifecycleState::Stopped: return "Stopped";
    case ServiceLifecycleState::Failed: return "Failed";
  }
  return "Unknown";
}

}  // namespace iris
