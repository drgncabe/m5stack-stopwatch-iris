#include "iris/apps/connectivity/MediaRemoteApp.h"

namespace iris {

void MediaRemoteApp::onStart() {
  if (bluetooth_.enabled() && !bluetooth_.connected() &&
      bluetooth_.bondedDeviceCount() > 0 && bluetooth_.autoReconnect()) {
    bluetooth_.startAdvertising();
  }
}

void MediaRemoteApp::onResume() {
  if (bluetooth_.enabled() && !bluetooth_.connected() &&
      bluetooth_.bondedDeviceCount() > 0 && bluetooth_.autoReconnect()) {
    bluetooth_.startAdvertising();
  }
}

}  // namespace iris
