/*
 * MotionTracker.cpp
 *
 *  Created on: Feb 7, 2017
 *      Author: rhb
 */

#include "MotionTracker.h"

MotionTracker::MotionTracker (const int32_t ringSize)
 : _ring(ringSize)
 , _sleepTimer(9930000, &MotionTracker::noActivity, *this, true)
 , _blinkTimer(10, &MotionTracker::turnLEDOff, *this, true)
 , _streamIntervalTimer(3000, &MotionTracker::stopStreaming, *this, true)
 , _streamingTimer(10, &MotionTracker::sampleStream, *this, false)
 , _lastActivityTime(0)
 , _boardLED(D7)
{
}

MotionTracker::~MotionTracker ()
{
  _sleepTimer.dispose();
  _blinkTimer.dispose();
  _streamIntervalTimer.dispose();
  _streamingTimer.dispose();
}

void
MotionTracker::begin()
{
  pinMode(_boardLED, OUTPUT);

  monitorAccelerometer();
  accelerometer.logControlRegs();

  Particle.function("sleep-time", &MotionTracker::setSleepTime, this);
  Particle.function("interval", &MotionTracker::setIntervalTime, this);
  Particle.function("streaming", &MotionTracker::setStreamingTime, this);
}

const int16_t
MotionTracker::upload(const int16_t hunk)
{
  return _ring.empty(hunk);
}

int
MotionTracker::setSleepTime(String command)
{
  return setTimer(command, _sleepTimer, "sleep");
}

int
MotionTracker::setIntervalTime(String command)
{
  return setTimer(command, _streamIntervalTimer, "stream-interval");
}

int
MotionTracker::setStreamingTime(String command)
{
  return setTimer(command, _streamingTimer, "streaming");
}

int
MotionTracker::setTimer(String command, Timer &timer, String name)
{
  int delay;

  Log.info("received new timer value '%s' for timer %s", command.c_str(), name.c_str());
  if (sscanf(command, "%d", &delay) != 1)
  {
    Log.warn("could not parse int from %s", command.c_str());
    return 0;
  }
  Log.info("new %s timer is %d", name.c_str(), delay);
  bool wasActive = timer.isActive();
  timer.changePeriod(delay);
  // leave timer in same running state as it was originally; .changePeriod() causes it to start
  if (!wasActive)
  {
    timer.stop();
  }

  return 1;
}

void
MotionTracker::sampleStream()
{
  static MotionEntry measurement;

  if (accelerometer.xyzReady())
  {
    accelerometer.xyz(measurement._x, measurement._y, measurement._z);
    measurement._time = Time.now();
    measurement._mode = 's';

    _ring.fill(measurement);
  }
}

void
MotionTracker::stopStreaming()
{
  _streamingTimer.stopFromISR();
  Log.info("done streaming");
}

void
MotionTracker::turnLEDOff()
{
  digitalWrite(_boardLED, LOW);
}

void
MotionTracker::suspendSelf()
{
  Log.info("going to sleep now");
  Serial.flush();
  detachInterrupt(D1);	// interrupt also wired to WKUP pin
  _sleepTimer.stop();

  accelerometer.sleepMode(5, 0x5, 0x0, LIS331::interrupt1, 0x2A);

#define DEEP_IS_BETTER
#ifdef DEEP_IS_BETTER
  System.sleep(SLEEP_MODE_DEEP, 60);
#else
  System.sleep(D1,RISING);

#ifdef NO_ISR_AFTER_SLEEP
  accelerometer.SPIwriteOneRegister(0x30, 0x00);  // clear interrupt axes
  accelerometer.SPIwriteOneRegister(0x20, 0x37);  // regular again
#endif
  monitorAccelerometer();
#endif
}

void
MotionTracker::noActivity()
{
  Log.info("looks like no more motion detected\n");
  suspendSelf();
}

void
MotionTracker::logEvery(const uint32_t dutyCycle)
{
 if (! _lastActivityTime)
 {
   _lastActivityTime = System.ticks();
 }
 else
 {
   uint32_t now = System.ticks();
   uint32_t duration = (now - _lastActivityTime )/System.ticksPerMicrosecond();

   if (duration > dutyCycle)
   {
     Log.info("activity!");
     //Serial.flush();
     _lastActivityTime = 0;
   }
 }
}

void
MotionTracker::blinkNotify()
{
  digitalWrite(_boardLED, HIGH);
  _blinkTimer.resetFromISR();
}

void
MotionTracker::motionDetected()
{
  logEvery(100000);
  blinkNotify();

  int16_t XData, YData, ZData;
  accelerometer.xyz(XData, YData, ZData);
  Log.trace("data: %d %d %d", XData, YData, ZData);
  MotionEntry measurement(Time.now(), 'i', XData, YData, ZData);
  _ring.fill(measurement);

  _sleepTimer.resetFromISR();
  _streamIntervalTimer.resetFromISR();
  _streamingTimer.resetFromISR();

  // clear edge, else ISR won't trigger again
  accelerometer.clearInterruptLatch(LIS331::interrupt1);
}

void
MotionTracker::monitorAccelerometer()
{
  pinMode(D1, INPUT);
  _sleepTimer.start();
  accelerometer.begin(SS);
  accelerometer.activityInterrupt(0x2, 0x01, LIS331::interrupt1, 0xAA);
  attachInterrupt(D1, &MotionTracker::motionDetected, this, RISING);
}
