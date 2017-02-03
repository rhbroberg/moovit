//#pragma once

#include "application.h"

class LIS331
{
public:

	LIS331();

	//
	// Basic Device control and readback functions
	//
	void begin(int16_t chipSelectPin = SS);
#ifdef NO_RHB
	int16_t readXData();
	int16_t readYData();
	int16_t readZData();
#endif
	void readXYZData(int16_t &XData, int16_t &YData, int16_t &ZData);

	//
	// Activity/Inactivity interrupt functions
	//
	void setupDCActivityInterrupt(int16_t threshold, byte time);
	void setupDCInactivityInterrupt(int16_t threshold, int16_t time);
	void setupACActivityInterrupt(int16_t threshold, byte time);
	void setupACInactivityInterrupt(int16_t threshold, int16_t time);

	void checkAllControlRegs();

	//  Low-level SPI control, to simplify overall coding
	byte SPIreadOneRegister(byte regAddress);
	void SPIwriteOneRegister(byte regAddress, byte regValue);
	int16_t  SPIreadTwoRegisters(byte regAddress);
	void SPIwriteTwoRegisters(byte regAddress, int16_t twoRegValue);

private:
	int16_t slaveSelectPin;
};

#include "application.h"
//#include "lis331.h"

// #define LIS331_DEBUG 1

#define SCALE 0.0007324  //sets g-level (48 fullscale range)/(2^16bits) = SCALE

LIS331::LIS331()
: slaveSelectPin(SS)
{
}

//
//  begin()
//  Initial SPI setup, soft reset of device
//
void
LIS331::begin(int16_t chipSelectPin)
{
  slaveSelectPin = chipSelectPin;
  pinMode(slaveSelectPin, OUTPUT);
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);	//CPHA = CPOL = 0    MODE = 0
  SPI.setBitOrder(MSBFIRST);
  delay(1000);

  // soft reset
  SPIwriteOneRegister(0x20, 0x37);  // normal mode, xyz-enabled
  delay(100);
  SPIwriteOneRegister(0x21, 0x00);  // hp filter off
  delay(100);
//  SPIwriteOneRegister(0x23, 0x30);  // 24g
  SPIwriteOneRegister(0x23, 0x00);  // 6g
  delay(100);

#ifdef LIS331_DEBUG
  Serial.println("begin\n");
#endif
}

#ifdef NO_RHB
//
//  readXData(), readYData(), readZData()
//  Read X, Y, Z registers
//
int16_t
LIS331::readXData()
{
  int16_t XDATA = SPIreadTwoRegisters(0x0E);

#ifdef LIS331_DEBUG
  Serial.print("XDATA = ");
  Serial.println(XDATA);
#endif

  return XDATA;
}

int16_t
LIS331::readYData()
{
  int16_t YDATA = SPIreadTwoRegisters(0x10);

#ifdef LIS331_DEBUG
  Serial.print("\tYDATA = ");
  Serial.println(YDATA);
#endif

  return YDATA;
}

int16_t
LIS331::readZData()
{
  int16_t ZDATA = SPIreadTwoRegisters(0x12);

#ifdef LIS331_DEBUG
  Serial.print("\tZDATA = ");
  Serial.println(ZDATA);
#endif

  return ZDATA;
}
#endif

void
LIS331::readXYZData(int16_t &XData, int16_t &YData, int16_t &ZData)
{
  // burst SPI read
  // A burst read of all three axis is required to guarantee all measurements correspond to same sample time
  digitalWrite(slaveSelectPin, LOW);
  SPI.transfer(0x80 | 0x40 | 0x28);  // read consecutive starting at 0x28
  XData = SPI.transfer(0x00);
  XData = XData + (SPI.transfer(0x00) << 8);

  YData = SPI.transfer(0x00);
  YData = YData + (SPI.transfer(0x00) << 8);

  ZData = SPI.transfer(0x00);
  ZData = ZData + (SPI.transfer(0x00) << 8);

  digitalWrite(slaveSelectPin, HIGH);

#ifdef LIS331_DEBUG
  Serial.print("XDATA = "); Serial.print(XData);
  Serial.print("\tYDATA = "); Serial.print(YData);
  Serial.print("\tZDATA = "); Serial.print(ZData);
#endif
}

void
LIS331::setupDCActivityInterrupt(int16_t threshold, byte time)
{
  //  Setup motion and time thresholds
  SPIwriteTwoRegisters(0x20, threshold);
  SPIwriteOneRegister(0x22, time);

  // turn on activity interrupt
  byte ACT_INACT_CTL_Reg = SPIreadOneRegister(0x27);  // Read current reg value
  ACT_INACT_CTL_Reg = ACT_INACT_CTL_Reg | (0x01);     // turn on bit 1, ACT_EN
  SPIwriteOneRegister(0x27, ACT_INACT_CTL_Reg);       // Write new reg value
  ACT_INACT_CTL_Reg = SPIreadOneRegister(0x27);       // Verify properly written

#ifdef LIS331_DEBUG
  Serial.print("DC Activity Threshold set to ");  	Serial.print(SPIreadTwoRegisters(0x20));
  Serial.print(", Time threshold set to ");  		Serial.print(SPIreadOneRegister(0x22));
  Serial.print(", ACT_INACT_CTL Register is ");  	Serial.println(ACT_INACT_CTL_Reg, HEX);
#endif
}

void
LIS331::setupACActivityInterrupt(int16_t threshold, byte time)
{
  //  Setup motion and time thresholds
  SPIwriteTwoRegisters(0x20, threshold);
  SPIwriteOneRegister(0x22, time);

  // turn on activity interrupt
  byte ACT_INACT_CTL_Reg = SPIreadOneRegister(0x27);  // Read current reg value
  ACT_INACT_CTL_Reg = ACT_INACT_CTL_Reg | (0x03);     // turn on bit 2 and 1, ACT_AC_DCB, ACT_EN
  SPIwriteOneRegister(0x27, ACT_INACT_CTL_Reg);       // Write new reg value
  ACT_INACT_CTL_Reg = SPIreadOneRegister(0x27);       // Verify properly written

#ifdef LIS331_DEBUG
  Serial.print("AC Activity Threshold set to ");  	Serial.print(SPIreadTwoRegisters(0x20));
  Serial.print(", Time Activity set to ");  		Serial.print(SPIreadOneRegister(0x22));
  Serial.print(", ACT_INACT_CTL Register is ");  Serial.println(ACT_INACT_CTL_Reg, HEX);
#endif
}

void
LIS331::setupDCInactivityInterrupt(int16_t threshold, int16_t time)
{
  // Setup motion and time thresholds
  SPIwriteTwoRegisters(0x23, threshold);
  SPIwriteTwoRegisters(0x25, time);

  // turn on inactivity interrupt
  byte ACT_INACT_CTL_Reg = SPIreadOneRegister(0x27);   // Read current reg value
  ACT_INACT_CTL_Reg = ACT_INACT_CTL_Reg | (0x04);      // turn on bit 3, INACT_EN
  SPIwriteOneRegister(0x27, ACT_INACT_CTL_Reg);        // Write new reg value
  ACT_INACT_CTL_Reg = SPIreadOneRegister(0x27);        // Verify properly written

#ifdef LIS331_DEBUG
  Serial.print("DC Inactivity Threshold set to ");  Serial.print(SPIreadTwoRegisters(0x23));
  Serial.print(", Time Inactivity set to ");  Serial.print(SPIreadTwoRegisters(0x25));
  Serial.print(", ACT_INACT_CTL Register is ");  Serial.println(ACT_INACT_CTL_Reg, HEX);
#endif
}

void
LIS331::setupACInactivityInterrupt(int16_t threshold, int16_t time)
{
  //  Setup motion and time thresholds
  SPIwriteTwoRegisters(0x23, threshold);
  SPIwriteTwoRegisters(0x25, time);

  // turn on inactivity interrupt
  byte ACT_INACT_CTL_Reg = SPIreadOneRegister(0x27);   // Read current reg value
  ACT_INACT_CTL_Reg = ACT_INACT_CTL_Reg | (0x0C);      // turn on bit 3 and 4, INACT_AC_DCB, INACT_EN
  SPIwriteOneRegister(0x27, ACT_INACT_CTL_Reg);        // Write new reg value
  ACT_INACT_CTL_Reg = SPIreadOneRegister(0x27);        // Verify properly written

#ifdef LIS331_DEBUG
  Serial.print("AC Inactivity Threshold set to ");  Serial.print(SPIreadTwoRegisters(0x23));
  Serial.print(", Time Inactivity set to ");  Serial.print(SPIreadTwoRegisters(0x25));
  Serial.print(", ACT_INACT_CTL Register is ");  Serial.println(ACT_INACT_CTL_Reg, HEX);
#endif
}

void
LIS331::checkAllControlRegs()
{
  //byte filterCntlReg = SPIreadOneRegister(0x2C);
  //byte ODR = filterCntlReg & 0x07;  Serial.print("ODR = ");  Serial.println(ODR, HEX);
  //byte ACT_INACT_CTL_Reg = SPIreadOneRegister(0x27);      Serial.print("ACT_INACT_CTL_Reg = "); Serial.println(ACT_INACT_CTL_Reg, HEX);
  digitalWrite(slaveSelectPin, LOW);
  SPI.transfer(0x80 | 0x40 | 0x20);  // read consecutive starting at 0x20
#ifdef LIS331_DEBUG
  Serial.println("Start Burst Read of all Control Regs");
  Serial.print("Reg 20 = "); 	Serial.println(SPI.transfer(0x00), HEX);
  Serial.print("Reg 21 = "); 	Serial.println(SPI.transfer(0x00), HEX);
  Serial.print("Reg 22 = "); 	Serial.println(SPI.transfer(0x00), HEX);
  Serial.print("Reg 23 = "); 	Serial.println(SPI.transfer(0x00), HEX);
  Serial.print("Reg 24 = "); 	Serial.println(SPI.transfer(0x00), HEX);
  Serial.print("Reg 25 = "); 	Serial.println(SPI.transfer(0x00), HEX);
  Serial.print("Reg 26 = "); 	Serial.println(SPI.transfer(0x00), HEX);
  Serial.print("Reg 27 = "); 	Serial.println(SPI.transfer(0x00), HEX);
#endif
  digitalWrite(slaveSelectPin, HIGH);
}

// Basic SPI routines to simplify code
// read and write one register
byte
LIS331::SPIreadOneRegister(byte regAddress)
{
  byte regValue = 0;

  digitalWrite(slaveSelectPin, LOW);
  SPI.transfer(regAddress | 0x80);  // read specifies 1 in top bit
  regValue = SPI.transfer(0x00);
  digitalWrite(slaveSelectPin, HIGH);

  return regValue;
}

void
LIS331::SPIwriteOneRegister(byte regAddress, byte regValue)
{
  digitalWrite(slaveSelectPin, LOW);
  SPI.transfer(regAddress);  // write specifes 0 in top bit, so just address
  SPI.transfer(regValue);
  digitalWrite(slaveSelectPin, HIGH);
}

int16_t
LIS331::SPIreadTwoRegisters(byte regAddress)
{
  int16_t twoRegValue = 0;

  digitalWrite(slaveSelectPin, LOW);
  SPI.transfer(regAddress | 0x80);  // read specifies 1 in top bit
  twoRegValue = SPI.transfer(0x00);
  twoRegValue = twoRegValue + (SPI.transfer(0x00) << 8);
  digitalWrite(slaveSelectPin, HIGH);

  return twoRegValue;
}

void
LIS331::SPIwriteTwoRegisters(byte regAddress, int16_t twoRegValue)
{
  byte twoRegValueH = twoRegValue >> 8;
  byte twoRegValueL = twoRegValue;

  digitalWrite(slaveSelectPin, LOW);  // write specifies 0 in top bit
  SPI.transfer(regAddress);
  SPI.transfer(twoRegValueL);
  SPI.transfer(twoRegValueH);
  digitalWrite(slaveSelectPin, HIGH);
}


// This #include statement was automatically added by the Particle IDE

// ------------
// Blink an LED
// ------------

/*-------------

We've heavily commented this code for you. If you're a pro, feel free to ignore it.

Comments start with two slashes or are blocked off by a slash and a star.
You can read them, but your device can't.
It's like a secret message just for you.

Every program based on Wiring (programming language used by Arduino, and Particle devices) has two essential parts:
setup - runs once at the beginning of your program
loop - runs continuously over and over

You'll see how we use these in a second.

This program will blink an led on and off every second.
It blinks the D7 LED on your Particle device. If you have an LED wired to D0, it will blink that LED as well.

-------------*/


// First, we're going to make some variables.
// This is our "shorthand" that we'll use throughout the program:

int led1 = D0; // Instead of writing D0 over and over again, we'll write led1
// You'll need to wire an LED to this one to see it blink.

int led2 = D7; // Instead of writing D7 over and over again, we'll write led2
// This one is the little blue LED on your board. On the Photon it is next to D7, and on the Core it is next to the USB jack.

// Having declared these variables, let's move on to the setup function.
// The setup function is a standard part of any microcontroller program.
// It runs only once when the device boots up or is reset.

LIS331 myaccelerometer;
SYSTEM_MODE(SEMI_AUTOMATIC);

void noActivity();

Timer timer(30000, noActivity, true);

void
suspendSelf()
{
    Serial.printf("see ya\n");
    Serial.flush();
    detachInterrupt(D1);
    timer.stop();

    myaccelerometer.SPIreadOneRegister(0x31);
    myaccelerometer.SPIwriteOneRegister(0x20, 0x57);  // sleep mode, 1 Hz low

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

volatile bool activitySeen = false;

void
activity()
{
  Serial.print("activity!\n");
  Serial.flush();
  activitySeen = true;
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

  myaccelerometer.SPIwriteOneRegister(0x22, 0x04);  // sleep mode latch int1  // 0x04 == latch
  myaccelerometer.SPIwriteOneRegister(0x30, 0xAA);  // sleep mode 'AND' mode for interrupt
// AND, 2 threshold, 0 duration, 1hz, ok
// OR, threshold 14, duration 1, 1hz - too sensitive; but threshold 15 not enuf
//    myaccelerometer.SPIwriteOneRegister(0x30, 0x2A);  // sleep mode 'OR' mode for interrupt

  myaccelerometer.SPIwriteOneRegister(0x32, 0x02);  // sleep mode threshold
  myaccelerometer.SPIwriteOneRegister(0x33, 0x00);  // sleep mode duration

//  timer = Timer(5000, noActivity);
}

void setup() {

  // We are going to tell our device that D0 and D7 (which we named led1 and led2 respectively) are going to be output
  // (That means that we will be sending voltage to them, rather than monitoring voltage that comes from them)

  // It's important you do this here, inside the setup() function rather than outside it or in the loop function.

  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);

  WiFi.off();

  // disable on-board RGB LED on Photon/Electron
  pinMode(RGBR, INPUT_PULLUP);
  pinMode(RGBG, INPUT_PULLUP);
  pinMode(RGBB, INPUT_PULLUP);

  Serial.print("done with setup\n");
  monitorAccelerometer();
}

// Next we have the loop function, the other essential part of a microcontroller program.
// This routine gets repeated over and over, as quickly as possible and as many times as possible, after the setup function is called.
// Note: Code that blocks for too long (like more than 5 seconds), can make weird things happen (like dropping the network connection).  The built-in delay function shown below safely interleaves required background activity, so arbitrarily long delays can safely be done if you need them.

int i = 1;
int j = 0;

void loop() {
#define BLINKY
#ifdef BLINKY
  // To blink the LED, first we'll turn it on...
  digitalWrite(led1, HIGH);
  digitalWrite(led2, HIGH);

  // We'll leave it on for 1 second...
  delay(1000);

  // Then we'll turn it off...
  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);
#endif

  int16_t XData, YData, ZData;
//  myaccelerometer.checkAllControlRegs();
  myaccelerometer.readXYZData(XData, YData, ZData);
  Serial.printf("data: %d %d %d\n", XData, YData, ZData);
  //myaccelerometer.SPIreadOneRegister(0x31);

  // Wait 1 second...
  delay(1000);

  if ((++i % 5) == 0)
  {
  }
}

