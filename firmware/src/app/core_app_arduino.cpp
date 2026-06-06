#include "app/core_app.h"

#include "core/platform/arduino_core_platform.h"
#include "drivers/onewire/onewire_bus.h"
#include "drivers/sensors/ds18b20/ds18b20_temperature_sensor.h"

namespace reeflow::app {

CoreApp& defaultCoreApp() {
  static drivers::sensors::Ds18b20TemperatureSensor temperatureSensor(
      drivers::arduinoOneWireBus(),
      core::platform::arduinoCorePlatform().timeSource());
  static CoreApp app(core::platform::arduinoCorePlatform(),
                     core::events::defaultEventBus(),
                     config::defaultConfigManager(), temperatureSensor);
  return app;
}

void setupCoreApp() {
  defaultCoreApp().setup();
}

void loopCoreApp() {
  defaultCoreApp().loopOnce();
}

}  // namespace reeflow::app
