#include "lis331.h"

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

