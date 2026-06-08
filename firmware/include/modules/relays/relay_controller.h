#pragma once

#include "modules/relays/relay_types.h"

namespace reeflow::modules::relays {

enum class RelayCommandResult {
  kSuccess,
  kUnknownRelay,
  kControllerFailure,
  kInvalidContract,
};

class RelayController {
 public:
  virtual ~RelayController() = default;

  virtual RelayCommandResult setRelay(RelayId relay,
                                      RelayDesiredState desiredState) = 0;
  virtual RelayCommandResult allOff() = 0;
};

}  // namespace reeflow::modules::relays
