#pragma once

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
