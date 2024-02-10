#ifndef BASE_TOUCH_BUTTON_H
#define BASE_TOUCH_BUTTON_H

#include <functional>

/*
  Arrived at this by experimentation. On battery power, the lack of ground plane
  require a higher threshold to register
*/
#define TOUCH_THRESHOLD 75

class TouchButtonBase {
public:
  // Make the destructor virtual since we have virtual functions
  virtual ~TouchButtonBase() {}

  virtual void init() = 0;

  virtual void process() = 0;

  virtual void setOnLongPressedCallback(std::function<void(void)> callback) = 0;

  virtual void setOnShortPressedCallback(std::function<void(void)> callback) = 0;
};

#endif // BASE_TOUCH_BUTTON_H