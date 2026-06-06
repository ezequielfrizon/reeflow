#pragma once

namespace reeflow::drivers {

class BringupTimer {
 public:
  virtual ~BringupTimer() = default;

  virtual void delayMs(unsigned long milliseconds) = 0;
};

}  // namespace reeflow::drivers
