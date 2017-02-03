#include "lis331.h"

// #define LIS331_DEBUG 1

#define SCALE 0.0007324  //sets g-level (48 fullscale range)/(2^16bits) = SCALE
#define OUT_X	0x28
#define OUT_Y	0x2A
#define OUT_Z	0x2C

LIS331::LIS331()
: _slaveSelectPin(SS)
{
}

//
//  begin()
//  Initial SPI setup, soft reset of device
//
void
LIS331::begin(int16_t chipSelectPin)
{
  _slaveSelectPin = chipSelectPin;
  pinMode(_slaveSelectPin, OUTPUT);
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);	//CPHA = CPOL = 0    MODE = 0
  SPI.setBitOrder(MSBFIRST);
//  delay(1000);

  // soft reset
  SPIwriteOneRegister(0x20, 0x37);  // normal mode, xyz-enabled
  //delay(100);
  SPIwriteOneRegister(0x21, 0x00);  // hp filter off
  //delay(100);
  //  SPIwriteOneRegister(0x23, 0x30);  // 24g
  SPIwriteOneRegister(0x23, 0x00);  // 6g
  //delay(100);

#ifdef LIS331_DEBUG
  Serial.println("begin\n");
#endif
}

const int16_t
LIS331::readXData() const
{
  int16_t XregValue = SPIreadTwoRegisters(OUT_X);

#ifdef LIS331_DEBUG
  Serial.print("XregValue = ");
  Serial.println(XregValue);
#endif

  return XregValue;
}

const int16_t
LIS331::readYData() const
{
  int16_t YregValue = SPIreadTwoRegisters(OUT_Y);

#ifdef LIS331_DEBUG
  Serial.print("\tYregValue = ");
  Serial.println(YregValue);
#endif

  return YregValue;
}

const int16_t
LIS331::readZData() const
{
  int16_t ZregValue = SPIreadTwoRegisters(OUT_Z);

#ifdef LIS331_DEBUG
  Serial.print("\tZregValue = ");
  Serial.println(ZregValue);
#endif

  return ZregValue;
}

void
LIS331::readXYZData(int16_t &XregValue, int16_t &YregValue, int16_t &ZregValue) const
{
  // burst SPI read
  // A burst read of all three axis is required to guarantee all measurements correspond to same sample time
  digitalWrite(_slaveSelectPin, LOW);
  SPI.transfer(0x80 | 0x40 | 0x28);  // read consecutive starting at 0x28
  XregValue = SPI.transfer(0x00);
  XregValue = XregValue + (SPI.transfer(0x00) << 8);

  YregValue = SPI.transfer(0x00);
  YregValue = YregValue + (SPI.transfer(0x00) << 8);

  ZregValue = SPI.transfer(0x00);
  ZregValue = ZregValue + (SPI.transfer(0x00) << 8);

  digitalWrite(_slaveSelectPin, HIGH);

#ifdef LIS331_DEBUG
  Serial.print("XregValue = "); Serial.print(XregValue);
  Serial.print("\tYregValue = "); Serial.print(YregValue);
  Serial.print("\tZregValue = "); Serial.print(ZregValue);
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
LIS331::logControlRegs()
{
  digitalWrite(_slaveSelectPin, LOW);
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
