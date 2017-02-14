/*
 * ActivityDigest.h
 *
 *  Created on: Feb 12, 2017
 *      Author: rhb
 */

#pragma once

#include "application.h"
#include "MotionEntry.h"

class ActivityDigest
{
  typedef struct ActiveMinute
  {
    time_t _time;
    int _entries;
  } ActiveMinute;

public:
  ActivityDigest ();
  virtual ~ActivityDigest ();

  void registerActivity(const MotionEntry &);
  const bool publishBacklog(const unsigned int entries);
  const unsigned int entries() const;
  const unsigned int capacity() const;
  const unsigned int remaining() const;
  void dump() const;

protected:
  const int timeOffset() const;
  static retained int _active;
  static retained int _lastUploaded;
  static retained time_t _lastActivity;
//  static retained ActiveMinute _minutes[60*24];
  static retained uint16_t _minutes[60*24];
  const unsigned int _capacity;
  const unsigned int _hunkSize;
};
