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
  delete[] _buffer;
}

const bool
NetworkRingBuffer::fill(const MotionEntry &entry)
{
  // must be atomic here; can be called from isr or from timer
  ATOMIC_BLOCK()
  {
    int32_t offset = (_head > _tail)? 0 : _length;

    if ((_head + offset) - _tail == 1)
    {
      Log.error("ring buffer full (%ld vs %ld).  too bad!", _head, _tail);
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

const int16_t
NetworkRingBuffer::empty(const int16_t hunkSize)
{
  int32_t startingTail;
  int32_t hunksSent = 0;

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
	if (!_client.connect("ec2-54-175-5-136.compute-1.amazonaws.com", 32768))
	{
	  Log.warn("cannot connect to aws");
	  return hunksSent;
	}
      }

      Log.info("starting backlog upload");
      int32_t ringIndex;
      for (int32_t i = _head; i < _head + hunkSize; i++)
      {
	ringIndex = i%_length;
	Log.trace("sending hunk %ld (index %ld) ", i, ringIndex);
	MotionEntry *entry = &_buffer[ringIndex];
	unsigned int lineSize = sprintf(_line, "%s,%c,%d,%d,%d\n", Time.format(entry->_time, TIME_FORMAT_ISO8601_FULL).c_str(), entry->_mode, entry->_x, entry->_y, entry->_z);

	if (lineSize >= sizeof(_line))
	{
	  Log.error("buffer overrun!  game over!");
	  System.reset();
	}

	if (unsigned int written = _client.write((const uint8_t *)_line, lineSize) != lineSize)
	{
	  Log.warn("network write partially failed; will try again later (%d/%d) (hunk %ld)", written, lineSize, i - _head);
	  break;
	}
	hunksSent++;
      }
      Log.trace("backlog sent");
      _client.stop();

      ATOMIC_BLOCK()
      {
	_head += hunksSent;
	_head %= _length;
      }
    }
    else
    {
      Log.trace("not enough to be troubled (%ld vs %ld)", _head, startingTail);
    }
  }
  return hunksSent;
}

const int16_t
NetworkRingBuffer::spaceLeft() const
{
  int16_t remainingEntries;

  ATOMIC_BLOCK()
  {
    int32_t offset = (_tail > _head) ? 0 : _length;
    remainingEntries = (_tail + offset) - _head;
  }

  return remainingEntries;
}
