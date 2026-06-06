#pragma once

#include <stddef.h>

#include <Arduino.h>

namespace reeflow::diagnostics {

void printInitialSafeHardwareState(Stream& output, size_t relaysConfigured,
                                   size_t pwmChannelsConfigured,
                                   size_t sensorBusPinsPrepared);

}  // namespace reeflow::diagnostics
