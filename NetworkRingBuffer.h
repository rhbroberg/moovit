/*
 * NetworkRingBuffer.h
 *
 *  Created on: Feb 6, 2017
 *      Author: rhb
 */

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

class NetworkRingBuffer
{
public:
  NetworkRingBuffer (const int32_t length);
  virtual ~NetworkRingBuffer ();

  const bool fill(const MotionEntry &);
  const int16_t empty(const int16_t hunkSize);
  const int16_t spaceLeft() const;

protected:
  TCPClient _client;
  MotionEntry *_buffer;  // 100 samples/sec for 30s
  int32_t _length;
  int32_t _head;
  int32_t _tail;
  char _line[128];
};
