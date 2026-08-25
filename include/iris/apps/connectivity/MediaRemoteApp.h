#pragma once

#include "iris/core/AppManager.h"
#include "iris/services/BluetoothService.h"

namespace iris {

class MediaRemoteApp : public IrisApplication {
 public:
  explicit MediaRemoteApp(BluetoothService& bluetooth) : bluetooth_(bluetooth) {}

  const char* id() const override { return "connectivity.mediaremote"; }
  const char* name() const override { return "Media Remote"; }
  void onStart() override;
  void onResume() override;

 private:
  BluetoothService& bluetooth_;
};

}  // namespace iris
