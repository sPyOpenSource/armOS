
/*
  Sensors.h - Library for Drone Sensors.
  Created by X. Wang, January 8, 2017.
  Released into the public domain.
*/
#ifndef Sensors_h
#define Sensors_h

#include "SFE_BMP180.h"
#include <LSM303.h>
#include <Adafruit_L3GD20.h>
#include <UnitTest.h>

class Sensors {
  public:
    double T = 20;  // in degree Celsius
    double P, height;
    VectorDouble gyr, acc, mag;
    void start();
    void read();

  private:
    SFE_BMP180 pressure;
    LSM303 lsm303;
    Adafruit_L3GD20 gyro;
    bool restartTemperature = true, restartPressure = true;
    char statusPressure, statusTemperature;
    uint32_t t0, t1;
    double baseline = 1013.2;  // in mbar
};

#endif
