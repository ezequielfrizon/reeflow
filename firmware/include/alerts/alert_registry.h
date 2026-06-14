#pragma once

#include "alerts/alert_types.h"

namespace reeflow::alerts {

struct AlertDefinition {
  AlertCode code;
  const char* name;
  AlertCategory category;
  AlertPriority defaultPriority;
  AlertRecoveryCode recoveryCode;
};

const AlertDefinition* alertDefinition(AlertCode code);
const AlertDefinition* alertDefinitionByIndex(size_t index);
size_t alertDefinitionCount();
AlertRecoveryCode recoveryForAlert(AlertCode code);
AlertCode alertForRecovery(AlertRecoveryCode recoveryCode);
const char* alertCodeName(AlertCode code);
const char* alertRecoveryName(AlertRecoveryCode recoveryCode);
const char* alertCategoryName(AlertCategory category);
const char* alertPriorityName(AlertPriority priority);
const char* alertSourceName(AlertSource source);

}  // namespace reeflow::alerts
