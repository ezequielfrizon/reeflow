#include "app/core_app.h"

#include "core/platform/arduino_core_platform.h"
#include "drivers/i2c/i2c_bus.h"
#include "drivers/onewire/onewire_bus.h"
#include "drivers/relays/gpio_relay_controller.h"
#include "drivers/io/arduino_hardware_io.h"
#include "drivers/sensors/ds18b20/ds18b20_temperature_sensor.h"
#include "drivers/sensors/vl6180x/vl6180x_level_sensor.h"
#include "modules/modes/mode_store.h"

namespace reeflow::app {

CoreApp& defaultCoreApp() {
  static drivers::sensors::Ds18b20TemperatureSensor temperatureSensor(
      drivers::arduinoOneWireBus(),
      core::platform::arduinoCorePlatform().timeSource());
  static drivers::sensors::Vl6180xLevelSensor waterLevelSensor(
      drivers::arduinoI2cBus());
  static drivers::GpioRelayController relayController(
      drivers::arduinoGpioPort());
  static modules::modes::VolatileModeStore modeStore;
  static CoreApp app(core::platform::arduinoCorePlatform(),
                     core::events::defaultEventBus(),
                     config::defaultConfigManager(), temperatureSensor,
                     waterLevelSensor, relayController, modeStore);
  return app;
}

void setupCoreApp() {
  defaultCoreApp().setup();
}

void loopCoreApp() {
  defaultCoreApp().loopOnce();
}

}  // namespace reeflow::app
