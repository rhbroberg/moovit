#include "lis331.h"

// #define LIS331_DEBUG 1

#define SCALE 0.0007324  //sets g-level (48 fullscale range)/(2^16bits) = SCALE

#define CTRL_REG1	0x20
#define CTRL_REG2	0x21
#define CTRL_REG3	0x22
#define CTRL_REG4	0x23
#define CTRL_REG5	0x24
#define HP_FILTER_RESET	0x25
#define REFERENCE	0x26
#define STATUS_REG	0x27
#define OUT_X_L		0x28
#define OUT_X_H		0x29
#define OUT_Y_L		0x2A
#define OUT_Y_H		0x2B
#define OUT_Z_L		0x2C
#define OUT_Z_H		0x2D
#define INT1_CFG	0x30
#define INT1_SOURCE	0x31
#define INT1_THS	0x32
#define INT1_DURATION	0x33
#define INT2_CFG	0x34
#define INT2_SOURCE	0x35
#define INT2_THS	0x36
#define INT2_DURATION	0x37

LIS331::LIS331()
: _slaveSelectPin(SS)
{
}

//
//  begin()
//  Initial SPI setup, soft reset of device
//
void
LIS331::begin(const int16_t chipSelectPin, const gScale g)
{
  _slaveSelectPin = chipSelectPin;
  pinMode(_slaveSelectPin, OUTPUT);
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);	//CPHA = CPOL = 0    MODE = 0
  SPI.setBitOrder(MSBFIRST);

  //SPIwriteOneRegister(CTRL_REG1, 0x20 | 0x07 | 0x08);  // normal mode | xyz-enabled | 100Hz
  SPIwriteOneRegister(CTRL_REG1, 0x20 | 0x07 | 0x10);  // normal mode | xyz-enabled | 400Hz
  SPIwriteOneRegister(CTRL_REG2, 0x10);  // hp filter on
  //  SPIwriteOneRegister(CTRL_REG4, 0x30);  // 24g
  SPIwriteOneRegister(CTRL_REG4, 0x80);  // 6g, block-data update, little-endian
}

const int16_t
LIS331::location(const byte start, const char axis) const
{
  int16_t value = SPIreadTwoRegisters(start);

  if (Log.isLevelEnabled(LOG_LEVEL_TRACE))
  {
    Log.trace("%c: %d", axis, value);
  }

  return value;
}

const int16_t
LIS331::x() const
{
  return location(OUT_X_L, 'x');
}

const int16_t
LIS331::y() const
{
  return location(OUT_Y_L, 'y');
}

const int16_t
LIS331::z() const
{
  return location(OUT_Z_L, 'z');
}

const bool
LIS331::xyzReady() const
{
  return SPIreadOneRegister(STATUS_REG) & 0x40;
}

void
LIS331::xyz(int16_t &x, int16_t &y, int16_t &z) const
{
  // burst SPI read
  // A burst read of all three axis is required to guarantee all measurements correspond to same sample time
  digitalWrite(_slaveSelectPin, LOW);
  SPI.transfer(0x80 | 0x40 | OUT_X_L);  // read consecutive starting at low byte of x register
  x = SPI.transfer(0x00);
  x = x + (SPI.transfer(0x00) << 8);

  y = SPI.transfer(0x00);
  y = y + (SPI.transfer(0x00) << 8);

  z = SPI.transfer(0x00);
  z = z + (SPI.transfer(0x00) << 8);

  digitalWrite(_slaveSelectPin, HIGH);

  if (Log.isLevelEnabled(LOG_LEVEL_TRACE))
  {
    Log.trace("x: %d; y: %d; z: %d", x, y, z);
  }
}

//accelerometer.SPIwriteOneRegister(0x30, 0x7F);  // interrupt mode movement mode
////  accelerometer.SPIwriteOneRegister(0x30, 0xEA);  // interrupt mode position mode
//accelerometer.SPIwriteOneRegister(0x32, 0x00);  // interrupt mode threshold
//accelerometer.SPIwriteOneRegister(0x33, 0x00);  // interrupt mode duration

void
LIS331::activityInterrupt(const byte threshold, const byte duration, const LIS331::pin which, const byte mode)
{
  // disable, configure parameters, attach interrupt, enable

  disableInterrupt(which);
  SPIwriteOneRegister(0x22, 0x00);	// disable latch, int1
  clearInterruptLatch(which);

  // AND, 2 threshold, 0 duration, 1hz, ok
  SPIwriteOneRegister(0x32, 0x02);  // interrupt mode threshold
  SPIwriteOneRegister(0x33, 0x01);  // interrupt mode duration
  SPIwriteOneRegister(0x22, 0x04);  // interrupt mode latch int1
  SPIwriteOneRegister(0x21, 0x1F);  // interrupt mode hpf

  enableInterrupt(which);;
}

void
LIS331::inactivityInterrupt(const byte threshold, const byte duration, const pin which, const byte mode)
{
//  _inActivityHandler[which] = completionHandler;
}

void
LIS331::disableInterrupt(const pin which)
{
  SPIwriteOneRegister(0x30, 0x00);	// disable interrupts
}

void
LIS331::enableInterrupt(const pin which)
{
  SPIwriteOneRegister(0x30, 0xAA);  // enable interrupt mode 'AND' mode
}

void
LIS331::clearInterruptLatch(const pin which)
{
  SPIreadOneRegister(0x31);  // clear any outstanding latch
}


void
LIS331::sleepMode(const byte frequency, const byte threshold, const byte duration, const pin which, const byte mode)
{
  SPIwriteOneRegister(0x20, 0xB7);  // sleep mode, 1 Hz low 0x57(0.5Hz) vs 0xB7 (5Hz)

  SPIwriteOneRegister(0x32, 0x05);  // interrupt mode threshold
  SPIwriteOneRegister(0x33, 0x00);  // interrupt mode duration
  SPIwriteOneRegister(0x30, 0x2A);  // interrupt mode 'OR' mode

  clearInterruptLatch(which);	// start fresh
}

void
LIS331::logControlRegs()
{
  digitalWrite(_slaveSelectPin, LOW);
  SPI.transfer(0x80 | 0x40 | 0x20);  // read consecutive starting at 0x20

  Serial.println("Start Burst Read of all Control Regs");
  Serial.print("Reg 20 = "); 	Serial.println(SPI.transfer(0x00), HEX);
  Serial.print("Reg 21 = "); 	Serial.println(SPI.transfer(0x00), HEX);
  Serial.print("Reg 22 = "); 	Serial.println(SPI.transfer(0x00), HEX);
  Serial.print("Reg 23 = "); 	Serial.println(SPI.transfer(0x00), HEX);
  Serial.print("Reg 24 = "); 	Serial.println(SPI.transfer(0x00), HEX);
  Serial.print("Reg 25 = "); 	Serial.println(SPI.transfer(0x00), HEX);
  Serial.print("Reg 26 = "); 	Serial.println(SPI.transfer(0x00), HEX);
  Serial.print("Reg 27 = "); 	Serial.println(SPI.transfer(0x00), HEX);

  digitalWrite(_slaveSelectPin, HIGH);
}

const byte
LIS331::SPIreadOneRegister(const byte regAddress) const
{
  byte regValue = 0;

  digitalWrite(_slaveSelectPin, LOW);
  SPI.transfer(0x80 | regAddress);
  regValue = SPI.transfer(0x00);
  digitalWrite(_slaveSelectPin, HIGH);

  return regValue;
}

const int16_t
LIS331::SPIreadTwoRegisters(const byte regAddress) const
{
  int16_t regValue = 0;

  digitalWrite(_slaveSelectPin, LOW);
  SPI.transfer(0x80 | 0x40 | regAddress);  // read consecutive starting at regAddress
  regValue = SPI.transfer(0x00);
  regValue += (SPI.transfer(0x00) << 8);
  digitalWrite(_slaveSelectPin, HIGH);

  return regValue;
}

void
LIS331::SPIwriteOneRegister(const byte regAddress, const byte regValue) const
{
  digitalWrite(_slaveSelectPin, LOW);
  SPI.transfer(regAddress);  // write specifies 0 in top bit, so just address
  SPI.transfer(regValue);
  digitalWrite(_slaveSelectPin, HIGH);
}

void
LIS331::SPIwriteTwoRegisters(const byte regAddress, const int16_t regValue) const
{
  byte regValueHigh = regValue >> 8;
  byte regValueLow = regValue;

  digitalWrite(_slaveSelectPin, LOW);
  SPI.transfer(regAddress);  // write specifies 0 in top bit, so just address
  SPI.transfer(regValueLow);
  SPI.transfer(regValueHigh);
  digitalWrite(_slaveSelectPin, HIGH);
}
