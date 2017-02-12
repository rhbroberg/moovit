#include "application.h"
#include "MotionTracker.h"

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

SerialLogHandler logHandler(LOG_LEVEL_INFO);
//SerialLogHandler logHandler(LOG_LEVEL_TRACE);
MotionTracker tracker(512, A1);	// use D1 or A1 (for all same side board connections).  using A0 conflicts with on-board LED; D0 conflicts as well
bool savePower = false;

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
maybeSetRTC()
{
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
}

void
setup()
{
  Serial.begin(115200);

  // disable on-board RGB LED on Photon/Electron
  pinMode(RGBR, INPUT_PULLUP);
  pinMode(RGBG, INPUT_PULLUP);
  pinMode(RGBB, INPUT_PULLUP);

  tracker.begin();

  if (savePower)
  {
    lowPowerMode();
  }
  else
  {
    Particle.connect();
  }
  Log.info("setup complete");
}

// Next we have the loop function, the other essential part of a microcontroller program.
// This routine gets repeated over and over, as quickly as possible and as many times as possible, after the setup function is called.
// Note: Code that blocks for too long (like more than 5 seconds), can make weird things happen (like dropping the network connection).  The built-in delay function shown below safely interleaves required background motionDetected, so arbitrarily long delays can safely be done if you need them.

void
loop()
{
  maybeSetRTC();
  (void) tracker.upload(128);
  delay(1000);
}
