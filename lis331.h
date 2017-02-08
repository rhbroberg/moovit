#pragma once

#include "application.h"

class LIS331
{
public:

  LIS331();

  enum gScale
  {
    g6 = 0,
    g12 = 1,
    g24 = 2
  };

  enum pin
  {
    interrupt1 = 0,
    interrupt2 = 1
  };

  // initialization and data reading
  void begin(const int16_t chipSelectPin = SS, const gScale g = g6);
  void setG(const gScale g = g6);

  const int16_t x() const;
  const int16_t y() const;
  const int16_t z() const;
  void xyz(int16_t &XData, int16_t &YData, int16_t &ZData) const;
  const bool xyzReady() const;

  // interrupt routines
  void activityInterrupt(const byte threshold, const byte duration, const pin which, const byte mode);
  void inactivityInterrupt(const byte threshold, const byte duration, const pin which, const byte mode);
  void disableInterrupt(const pin which);
  void enableInterrupt(const pin which);
  void clearInterruptLatch(const pin which);
  void sleepMode(const byte frequency, const byte threshold, const byte duration, const pin which, const byte mode);

  void logControlRegs();

private:
  const int16_t location(const byte start, const char axis) const;

  // Low-level SPI control, to simplify overall coding
  const byte SPIreadOneRegister(const byte regAddress) const;
  void SPIwriteOneRegister(const byte regAddress, const byte regValue) const;
  const int16_t SPIreadTwoRegisters(const byte regAddress) const;
  void SPIwriteTwoRegisters(const byte regAddress, const int16_t twoRegValue) const;

  int16_t _slaveSelectPin;
  std::function<void()> _activityHandler[2];
  std::function<void()> _inActivityHandler[2];
};
