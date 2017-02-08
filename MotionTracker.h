/*
 * MotionTracker.h
 *
 *  Created on: Feb 7, 2017
 *      Author: rhb
 */

#pragma once

#include "application.h"
#include "lis331.h"
#include "NetworkRingBuffer.h"

class MotionTracker
{
public:
  MotionTracker (NetworkRingBuffer &);
  virtual ~MotionTracker ();
  void begin();

  // probably need methods to tweak timeouts

protected:
  void blinkNotify();
  void logEvery(const uint32_t);
  void monitorAccelerometer();
  void motionDetected();
  void noActivity();
  void sampleStream();
  void stopStreaming();
  void suspendSelf();
  void turnLEDOff();

  NetworkRingBuffer &_ring;
  LIS331 accelerometer;
  Timer _sleepTimer;
  Timer _blinkTimer;
  Timer _streamIntervalTimer;
  Timer _streamingTimer;

  volatile uint32_t _lastActivityTime;
  int _boardLED;
};
