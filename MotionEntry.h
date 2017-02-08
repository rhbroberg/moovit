#pragma once

#include "application.h"

class MotionEntry
{
public:
  MotionEntry()
  : _time(0)
  , _mode('x')
  , _x(0)
  , _y(0)
  , _z(0)
  {
  }

  MotionEntry(const time_t &time, const char mode, const int16_t x, const int16_t y, const int16_t z)
  : _time(time)
  , _mode(mode)
  , _x(x)
  , _y(y)
  , _z(z)
  {
  }

  time_t _time;
  char _mode;
  int16_t _x,_y,_z;
};

