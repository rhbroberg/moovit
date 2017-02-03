#include "application.h"
#include "lis331.h"


// First, we're going to make some variables.
// This is our "shorthand" that we'll use throughout the program:

int led2 = D7; // Instead of writing D7 over and over again, we'll write led2
// This one is the little blue LED on your board. On the Photon it is next to D7, and on the Core it is next to the USB jack.

// Having declared these variables, let's move on to the setup function.
// The setup function is a standard part of any microcontroller program.
// It runs only once when the device boots up or is reset.

LIS331 myaccelerometer;

// during development, leave radio on for OTA
//#define MINIMAL_POWER_USE
#ifdef MINIMAL_POWER_USE
SYSTEM_MODE(SEMI_AUTOMATIC);
#endif

void noActivity();

void
turnLEDOff()
{
  digitalWrite(led2, LOW);
}

Timer timer(10000, noActivity, true);
Timer blinker(10, turnLEDOff, true);

void
suspendSelf()
{
  Serial.printf("see ya\n");
  Serial.flush();
  detachInterrupt(D1);
  timer.stop();

  myaccelerometer.SPIreadOneRegister(0x31);
  myaccelerometer.SPIwriteOneRegister(0x20, 0xB7);  // sleep mode, 1 Hz low 0x57(0.5Hz) vs 0xB7 (5Hz)

  myaccelerometer.SPIwriteOneRegister(0x30, 0x2A);  // interrupt mode 'OR' mode
  myaccelerometer.SPIwriteOneRegister(0x32, 0x04);  // interrupt mode threshold
  myaccelerometer.SPIwriteOneRegister(0x33, 0x00);  // interrupt mode duration

#define DEEP_IS_BETTER
#ifdef DEEP_IS_BETTER
  System.sleep(SLEEP_MODE_DEEP, 60);
#else
  System.sleep(D1,RISING);

#ifdef NO_ISR_AFTER_SLEEP
  myaccelerometer.SPIwriteOneRegister(0x30, 0x00);  // clear interrupt axes
  myaccelerometer.SPIwriteOneRegister(0x20, 0x37);  // regular again
#endif

  monitorAccelerometer();

#endif
}

void
noActivity()
{
  Serial.print("looks like no more activity\n");
  suspendSelf();
}

volatile uint32_t lastActivity = 0;

void
activity()
{
  if (! lastActivity)
  {
    lastActivity = System.ticks();
  }
  else
  {
    uint32_t now = System.ticks();
    uint32_t duration = (now - lastActivity )/System.ticksPerMicrosecond();

    if (duration > 100000)
    {
      Serial.print("activity!\n");
      //Serial.flush();
      lastActivity = 0;
    }
  }

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
  // clear edge
  myaccelerometer.SPIreadOneRegister(0x31);
}

void
monitorAccelerometer()
{
  myaccelerometer.begin(SS);

  // configure interrupt1

  myaccelerometer.SPIwriteOneRegister(0x22, 0x00);
  myaccelerometer.SPIwriteOneRegister(0x30, 0x00);
  myaccelerometer.SPIreadOneRegister(0x31);

  timer.start();
  pinMode(D1, INPUT);
  attachInterrupt(D1, activity, RISING);

  myaccelerometer.SPIwriteOneRegister(0x22, 0x04);  // interrupt mode latch int1  // 0x04 == latch
  myaccelerometer.SPIwriteOneRegister(0x21, 0x1F);  // interrupt mode hpf
#define AND_ACCELEROMETER
#ifdef AND_ACCELEROMETER
  // AND, 2 threshold, 0 duration, 1hz, ok
  myaccelerometer.SPIwriteOneRegister(0x30, 0xAA);  // interrupt mode 'AND' mode
  //  myaccelerometer.SPIwriteOneRegister(0x32, 0x06);  // interrupt mode threshold
  myaccelerometer.SPIwriteOneRegister(0x32, 0x02);  // interrupt mode threshold
  myaccelerometer.SPIwriteOneRegister(0x33, 0x01);  // interrupt mode duration
#else
#ifdef OR_ACCELEROMETER
  // OR, threshold 14, duration 1, 1hz - too sensitive; but threshold 15 not enuf
  myaccelerometer.SPIwriteOneRegister(0x30, 0x2A);  // interrupt mode 'OR' mode
  myaccelerometer.SPIwriteOneRegister(0x32, 0x15);  // interrupt mode threshold
  myaccelerometer.SPIwriteOneRegister(0x33, 0x01);  // interrupt mode duration
#else
  myaccelerometer.SPIwriteOneRegister(0x30, 0x7F);  // interrupt mode movement mode
  //  myaccelerometer.SPIwriteOneRegister(0x30, 0xEA);  // interrupt mode position mode
  myaccelerometer.SPIwriteOneRegister(0x32, 0x00);  // interrupt mode threshold
  myaccelerometer.SPIwriteOneRegister(0x33, 0x00);  // interrupt mode duration
#endif
#endif

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

void setup() {

  // We are going to tell our device that D0 and D7 (which we named led1 and led2 respectively) are going to be output
  // (That means that we will be sending voltage to them, rather than monitoring voltage that comes from them)

  // It's important you do this here, inside the setup() function rather than outside it or in the loop function.
  // Serial.begin(115200);

  pinMode(led2, OUTPUT);

  // disable on-board RGB LED on Photon/Electron
  pinMode(RGBR, INPUT_PULLUP);
  pinMode(RGBG, INPUT_PULLUP);
  pinMode(RGBB, INPUT_PULLUP);

  Serial.print("done with setup\n");
  monitorAccelerometer();
  myaccelerometer.logControlRegs();

#ifdef MINIMAL_POWER_USE
  lowPowerMode();
#endif
}

// Next we have the loop function, the other essential part of a microcontroller program.
// This routine gets repeated over and over, as quickly as possible and as many times as possible, after the setup function is called.
// Note: Code that blocks for too long (like more than 5 seconds), can make weird things happen (like dropping the network connection).  The built-in delay function shown below safely interleaves required background activity, so arbitrarily long delays can safely be done if you need them.

void loop() {
  int16_t XData, YData, ZData;
  myaccelerometer.readXYZData(XData, YData, ZData);
  Serial.printf("data: %d %d %d\n", XData, YData, ZData);

  // Wait 1 second...
  delay(1000);
}

