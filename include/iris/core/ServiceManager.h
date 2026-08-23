#pragma once

#include <Arduino.h>

namespace iris {

enum class ServiceLifecycleState : uint8_t {
  Registered,
  Started,
  Suspended,
  Stopped,
  Failed,
};

class IrisService {
 public:
  virtual ~IrisService() = default;

  virtual const char* id() const = 0;
  virtual const char* name() const = 0;
  virtual bool begin() { return true; }
  virtual void update(uint32_t) {}
  virtual void suspend() {}
  virtual void resume() {}
  virtual void stop() {}
};

struct ServiceDescriptor {
  constexpr ServiceDescriptor()
      : id(nullptr),
        name(nullptr),
        started(false),
        state(ServiceLifecycleState::Registered),
        service(nullptr),
        instance(nullptr) {}

  constexpr ServiceDescriptor(const char* serviceId,
                              const char* serviceName,
                              bool serviceStarted = true,
                              IrisService* managedService = nullptr,
                              void* serviceInstance = nullptr)
      : id(serviceId),
        name(serviceName),
        started(serviceStarted),
        state(serviceStarted ? ServiceLifecycleState::Started : ServiceLifecycleState::Registered),
        service(managedService),
        instance(serviceInstance) {}

  const char* id;
  const char* name;
  bool started;
  ServiceLifecycleState state;
  IrisService* service;
  void* instance;
};

class ServiceManager {
 public:
  bool registerService(const char* id, const char* name, bool started = true);
  bool registerService(const char* id, const char* name, void* instance, bool started = true);
  bool registerManagedService(IrisService* service, bool startImmediately = true);
  bool begin();
  void update(uint32_t nowMs);
  void suspend(const char* id);
  void resume(const char* id);
  void stop(const char* id);
  void setStarted(const char* id, bool started);

  const ServiceDescriptor* service(size_t index) const;
  const ServiceDescriptor* find(const char* id) const;
  const char* stateName(const char* id) const;
  size_t count() const { return count_; }
  size_t startedCount() const;
  String summary() const;

  template <typename T>
  T* get(const char* id) const {
    const ServiceDescriptor* descriptor = find(id);
    return descriptor ? static_cast<T*>(descriptor->instance) : nullptr;
  }

 private:
  static constexpr size_t kMaxServices = 18;

  ServiceDescriptor* findMutable(const char* id);
  bool beginDescriptor(ServiceDescriptor& descriptor);

  ServiceDescriptor services_[kMaxServices]{};
  size_t count_ = 0;
};

const char* serviceLifecycleStateName(ServiceLifecycleState state);

}  // namespace iris
