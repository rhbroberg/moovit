#include "application.h"
#include "lis331.h"
#include "NetworkRingBuffer.h"

void noActivity();
void monitorAccelerometer();
void turnLEDOff();
void stopStreaming();
void sampleMe();

NetworkRingBuffer entries(1024);
SerialLogHandler logHandler(LOG_LEVEL_INFO);
//SerialLogHandler logHandler(LOG_LEVEL_TRACE);
LIS331 accelerometer;
volatile uint32_t lastActivity = 0;
Timer timer(9930000, noActivity, true);
Timer blinker(10, turnLEDOff, true);
Timer streamData(3000, stopStreaming, true);
Timer streamIt(10, sampleMe, false);

int led2 = D7; // Instead of writing D7 over and over again, we'll write led2
bool savePower = false;

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

void
sampleMe()
{
  static MotionEntry measurement;

  accelerometer.xyz(measurement._x, measurement._y, measurement._z);
  measurement._time = Time.now();
  measurement._mode = 's';

  entries.fill(measurement);
}

void
stopStreaming()
{
  streamIt.stopFromISR();
  Log.info("done streaming");
}

void
turnLEDOff()
{
  digitalWrite(led2, LOW);
}

void
suspendSelf()
{
  Log.info("see ya\n");
  Serial.flush();
  detachInterrupt(D1);	// interrupt also wired to WKUP pin
  timer.stop();

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
noActivity()
{
  Log.info("looks like no more motionDetected\n");
  suspendSelf();
}

void
logEvery(const uint32_t dutyCycle)
{
 if (! lastActivity)
 {
   lastActivity = System.ticks();
 }
 else
 {
   uint32_t now = System.ticks();
   uint32_t duration = (now - lastActivity )/System.ticksPerMicrosecond();

   if (duration > dutyCycle)
   {
     Log.info("activity!");
     //Serial.flush();
     lastActivity = 0;
   }
 }
}

void
blinkNotify()
{
  digitalWrite(led2, HIGH);
  if (blinker.isActive())
  {
    blinker.resetFromISR();
  }
  else
  {
    blinker.startFromISR();
  }

  if (timer.isActive())
  {
    timer.resetFromISR();
  }
}

void
motionDetected()
{
  logEvery(100000);
  blinkNotify();

  int16_t XData, YData, ZData;
  accelerometer.xyz(XData, YData, ZData);
  Log.trace("data: %d %d %d", XData, YData, ZData);

  MotionEntry measurement(Time.now(), 'i', XData, YData, ZData);
  entries.fill(measurement);

  if (streamData.isActive())
  {
    streamData.resetFromISR();
  }
  else
  {
    streamData.startFromISR();
  }
  streamIt.resetFromISR();

  // clear edge, else ISR won't trigger again
  accelerometer.clearInterruptLatch(LIS331::interrupt1);
}

void
monitorAccelerometer()
{
  pinMode(D1, INPUT);
  timer.start();
  accelerometer.begin(SS);
  accelerometer.activityInterrupt(0x2, 0x01, LIS331::interrupt1, 0xAA);
  attachInterrupt(D1, motionDetected, RISING);
}

void
lowPowerMode()
{
  RCC_PCLK1Config(RCC_HCLK_Div1);
  RCC_PCLK2Config(RCC_HCLK_Div1);
  RCC_HCLKConfig(RCC_SYSCLK_Div64);

  SystemCoreClockUpdate();
  SysTick_Configuration();

  RGB.control(true);
  RGB.color(0, 0, 0);

  FLASH->ACR &= ~FLASH_ACR_PRFTEN;
}

void
setup()
{
  // We are going to tell our device that D0 and D7 (which we named led1 and led2 respectively) are going to be output
  // (That means that we will be sending voltage to them, rather than monitoring voltage that comes from them)

  Serial.begin(115200);

  pinMode(led2, OUTPUT);

  // disable on-board RGB LED on Photon/Electron
  pinMode(RGBR, INPUT_PULLUP);
  pinMode(RGBG, INPUT_PULLUP);
  pinMode(RGBB, INPUT_PULLUP);

  monitorAccelerometer();
  accelerometer.logControlRegs();

  if (savePower)
  {
    lowPowerMode();
  }
  else
  {
    Particle.connect();
  }
  Log.info("done with setup");
}

// Next we have the loop function, the other essential part of a microcontroller program.
// This routine gets repeated over and over, as quickly as possible and as many times as possible, after the setup function is called.
// Note: Code that blocks for too long (like more than 5 seconds), can make weird things happen (like dropping the network connection).  The built-in delay function shown below safely interleaves required background motionDetected, so arbitrarily long delays can safely be done if you need them.

void
loop()
{
#ifdef NO_MORE
  int16_t XData, YData, ZData;
  accelerometer.xyz(XData, YData, ZData);
  Log.info("data: %d %d %d", XData, YData, ZData);
#endif
  //Log.info("memory: %ld", System.freeMemory());

  // reset RTC if in low power mode and haven't connected to Particle yet
  if (savePower)
  {
    if (!Particle.connected() && (Time.year() == 1970))
    {
      Log.info("RTC not set; connecting up to set it");
      Particle.connect();
    }

    if (Particle.connected() && Time.year() != 1970)
    {
      Log.info("RTC now set; disconnecting from Particle");
      Particle.disconnect();
    }
  }

  delay(1000);
  entries.empty(256);
}
