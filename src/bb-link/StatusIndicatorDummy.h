
#include "StatusIndicatorBase.h"

class StatusIndicatorDummy : public StatusIndicatorBase
{
public:
  StatusIndicatorDummy() {}

  // Override init with an empty body
  void init() override {}

  // Override set with an empty body
  void set(status_t status) override {}

  // Override render with an empty body
  void render() override {}

  // Override sleep with an empty body
  void sleep() override {}
};
