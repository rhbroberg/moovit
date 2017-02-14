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
#include "ActivityDigest.h"

class MotionTracker
{
public:
  MotionTracker (const int32_t ringSize, const int pin);
  virtual ~MotionTracker ();
  void begin();
  const int16_t upload(const int16_t);

//protected:
  void button_handler(system_event_t event, int duration, void* );
  int setTimer(String command, Timer &timer, String name);
  int setSleepTime(String);
  int setIntervalTime(String);
  int setStreamingTime(String);

  void blinkNotify();
  void logEvery(const uint32_t);
  void monitorAccelerometer();
  void motionDetected();
  void noActivity();
  void sampleStream();
  void stopStreaming();
  void suspendSelf();
  void turnLEDOff();
  void publishDigest();

  NetworkRingBuffer _ring;
  LIS331 accelerometer;
  Timer _sleepTimer;
  Timer _blinkTimer;
  Timer _streamIntervalTimer;
  Timer _streamingTimer;
  Timer _publishDigestTimer;
  ActivityDigest _digest;

  volatile uint32_t _lastActivityTime;
  int _boardLED;
  int _interruptPin;
};
