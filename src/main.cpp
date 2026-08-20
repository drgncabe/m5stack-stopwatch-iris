#include <Arduino.h>
#include "iris/App.h"

iris::App app;

void setup() {
  app.begin();
}

void loop() {
  app.update();
}
