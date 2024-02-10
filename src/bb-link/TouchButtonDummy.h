#include "TouchButtonBase.h"

class TouchButtonDummy : public TouchButtonBase {
public:
  void init() override {}
  void process() override {}
  void setOnLongPressedCallback(std::function<void(void)> callback) override {}
  void setOnShortPressedCallback(std::function<void(void)> callback) override {}
};