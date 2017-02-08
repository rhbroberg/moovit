/*
 * NetworkRingBuffer.h
 *
 *  Created on: Feb 6, 2017
 *      Author: rhb
 */

#pragma once

#include "application.h"
#include "MotionEntry.h"

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
  MotionEntry *_buffer;
  int32_t _length;
  int32_t _head;
  int32_t _tail;
  char _line[128];
};
