#pragma once

#include <stddef.h>

#include "resilience/resilience_types.h"

namespace reeflow::resilience {

size_t resilienceScenarioCount();
const ResilienceScenarioDefinition& resilienceScenarioAt(size_t index);
const ResilienceScenarioDefinition* findResilienceScenario(
    ResilienceScenarioId id);
const char* resilienceScenarioName(ResilienceScenarioId id);

}  // namespace reeflow::resilience
