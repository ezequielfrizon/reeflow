#include <Arduino.h>

#include "app/core_app.h"

void setup() {
  reeflow::app::setupCoreApp();
}

void loop() {
  reeflow::app::loopCoreApp();
}
