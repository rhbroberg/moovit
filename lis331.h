#pragma once

#include "application.h"

class LIS331
{
public:

  LIS331();

  // initialization and data reading
  void begin(int16_t chipSelectPin = SS);
  const int16_t readXData() const;
  const int16_t readYData() const;
  const int16_t readZData() const;
  void readXYZData(int16_t &XData, int16_t &YData, int16_t &ZData) const;

  //
  // Activity/Inactivity interrupt functions
  //
  void setupDCActivityInterrupt(int16_t threshold, byte time);
  void setupDCInactivityInterrupt(int16_t threshold, int16_t time);
  void setupACActivityInterrupt(int16_t threshold, byte time);
  void setupACInactivityInterrupt(int16_t threshold, int16_t time);

  void logControlRegs();

  //  Low-level SPI control, to simplify overall coding
  const byte SPIreadOneRegister(const byte regAddress) const;
  void SPIwriteOneRegister(const byte regAddress, const byte regValue) const;
  const int16_t SPIreadTwoRegisters(const byte regAddress) const;
  void SPIwriteTwoRegisters(const byte regAddress, const int16_t twoRegValue) const;

private:
  int16_t _slaveSelectPin;
};
