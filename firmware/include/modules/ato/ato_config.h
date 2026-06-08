#pragma once

#include <stdint.h>

namespace reeflow::modules::ato {

struct AtoModuleConfig {
  uint32_t evaluationIntervalMillis;
  uint16_t canonicalMinimumLevel;
  uint16_t canonicalMaximumLevel;
};

constexpr AtoModuleConfig makeDefaultAtoModuleConfig() {
  return {
      1000,
      0,
      100,
  };
}

}  // namespace reeflow::modules::ato
