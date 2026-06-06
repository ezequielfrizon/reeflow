#include <Arduino.h>

#include "app/hardware_bringup_app.h"

void setup() {
  reeflow::app::setupHardwareBringupApp();
}

void loop() {
  reeflow::app::loopHardwareBringupApp();
}
