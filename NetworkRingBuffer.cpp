/*
 * NetworkRingBuffer.cpp
 *
 *  Created on: Feb 6, 2017
 *      Author: rhb
 */

#include "NetworkRingBuffer.h"

NetworkRingBuffer::NetworkRingBuffer (const int32_t length)
  : _length(length)
  , _head(0)
  , _tail(0)
{
  _buffer = new MotionEntry[_length];
}

NetworkRingBuffer::~NetworkRingBuffer ()
{
  // TODO Auto-generated destructor stub
}

const bool
NetworkRingBuffer::fill(const MotionEntry &entry)
{
//  ATOMIC_BLOCK()
  {
    // doesn't handle wrap
    int32_t offset = (_head > _tail)? 0 : _length;

    if ((_head + offset) - _tail == 1)
    {
      Log.warn("ring buffer full (%ld vs %ld).  too bad!", _head, _tail);
      return false;
    }
    else
    {
      Log.trace("plenty of room at the inn");
    }
    _buffer[_tail] = entry;
    _tail ++;
    _tail %= _length;
  }

  return true;
}

void
NetworkRingBuffer::empty(const int16_t hunkSize)
{
  int32_t startingTail;

  ATOMIC_BLOCK()
  {
    startingTail = _tail;
  }

  int32_t offset = (startingTail > _head)? 0 : _length;

  if (startingTail != _head)
  {
    Log.trace("there is work to be done");

    if ((startingTail + offset) - _head > hunkSize)
    {
      if (!_client.connected())
      {
	Log.info("connecting to aws");
	_client.connect("ec2-54-175-5-136.compute-1.amazonaws.com", 32768);
      }

      Log.info("time for sending");
      int32_t ringIndex;
      for (int32_t i = _head; i < _head + hunkSize; i++)
      {
	ringIndex = i%_length;
	Log.trace("sending hunk %ld (index %ld) ", i, ringIndex);
	MotionEntry *entry = &_buffer[ringIndex];
	sprintf(_line, "%s,%c,%d,%d,%d\n", Time.format(entry->_time, TIME_FORMAT_ISO8601_FULL).c_str(), entry->_mode, entry->_x, entry->_y, entry->_z);
	_client.write((const uint8_t *)_line, strlen(_line));
      }
      Log.trace("done sending, closing now");
      // _client.flush();
      _client.stop();

      ATOMIC_BLOCK()
      {
	_head += hunkSize;
	_head %= _length;
      }
    }
    else
    {
      Log.trace("(easy) - not enough to be troubled (%ld vs %ld)", _head, startingTail);
    }
  }
}
