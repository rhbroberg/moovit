/*
 * ActivityDigest.cpp
 *
 *  Created on: Feb 12, 2017
 *      Author: rhb
 */

#include "ActivityDigest.h"

 retained int ActivityDigest::_active(-1);
 retained int ActivityDigest::_lastUploaded(-1);
 retained time_t ActivityDigest::_lastActivity(0);
 retained uint16_t ActivityDigest::_minutes[60*24];
// retained ActivityDigest::ActiveMinute ActivityDigest::_minutes[60*24];

ActivityDigest::ActivityDigest ()
 : _capacity(sizeof(_minutes) / sizeof(uint16_t))
 , _hunkSize(30)
{
}

ActivityDigest::~ActivityDigest ()
{
}

const int
ActivityDigest::timeOffset() const
{
  // Time class is missing some signatures to make this more straightforward, e.g. 'Time now; ... offset = now.hour() + ...' would be a mroe conventional way
  time_t sinceEpoch = Time.now();
  int offset = Time.hour(sinceEpoch) * 60 + Time.minute(sinceEpoch);

  return offset;
}

void
ActivityDigest::registerActivity(const MotionEntry &motion)
{
  int offset = timeOffset();
  if (_active == -1)
  {
    _active = offset;
    _lastUploaded = offset;
    // _lastActivity = now;
  }

  _minutes[offset]++;
  _active = offset;
  Log.info("minutes[%d] is now %d", offset, _minutes[offset]);
}

void
ActivityDigest::dump() const
{
  int offset = timeOffset();

  Log.info("Digest info:");
  for (int i = 0; i <= offset; i++ )
  {
    Log.info("slot[%d] = %d", i, _minutes[i]);
  }
}

const bool
ActivityDigest::publishBacklog(const unsigned int entries)
{
  // bulk publish of backlog
  // offset:count,count,...count
  // 4+1 5+1 5+1 ... 5+1
  // 255 = 5 + N * 6; N = 250/6 = 41
  // so 2 publishes of 30 items each is 1 hour of data; 48 seconds for 1 day
  //
  // individual backlog publish: each hour 2 publishes
  // expose function to publish now
  // expose function to set publish rate
  static char publishBuf[128];

  // wait for 10s to attach to cloud; should be configurable
  if (!waitFor(Particle.connected, 10000))
  {
    Log.warn("bummer, can't connect to cloud right not, try again later");
    return false;
  }

  // must disable sleep timer during this backlog update - fix
  unsigned int initialActive;
  ATOMIC_BLOCK()
  {
    initialActive = _active;
  }

  unsigned int initialLastUploaded = _lastUploaded;
  byte messageLength = 0;
  for (unsigned int i = 0; i < entries; i++)
  {
    uint16_t minuteSummary;
    unsigned int timeOffset = (initialLastUploaded + 1 + i) % _capacity;
    minuteSummary = _minutes[timeOffset];

    if (messageLength == 0)
    {
      memset(publishBuf, 0, sizeof(publishBuf));
      messageLength += sprintf(publishBuf, "%d:%d", timeOffset, minuteSummary);
    }
    else
    {
      messageLength += sprintf(&publishBuf[messageLength], ",%d", minuteSummary);
    }
    Log.info("using index %d, messageLength = %d", timeOffset, messageLength);

    if ((messageLength > (sizeof(publishBuf) - 6 - 1)) // buffer - (max size of printed uint16_t) - (leave space for NULL terminator)
	|| (i + 1 >= entries))
    {
      Log.info("going to publish '%s'", publishBuf);
      if (!Particle.connected() || (Particle.publish("activity", publishBuf) == false))
      {
	Log.info("publish failed; leaving backlog, last minute uploaded was %d", _lastUploaded);
	return false;
      }
      // particle.io mqtt throttles at 1/sec
      delay(1000);
      _lastUploaded = timeOffset;
      messageLength = 0;
    }
  }

  return true;
}

const unsigned int
ActivityDigest::entries() const
{
  return ((_active - _lastUploaded) + _capacity) % _capacity;
}

const unsigned int
ActivityDigest::capacity() const
{
  return _capacity;
}

const unsigned int
ActivityDigest::remaining() const
{
  return capacity() - entries();
}
