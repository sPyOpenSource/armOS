
/*
  Sensors.h - Library for Drone Sensors.
  Created by X. Wang, January 8, 2017.
  Released into the public domain.
*/
#ifndef Sensors_h
#define Sensors_h

#include "SFE_BMP180.h"

class Sensors {
  public:
    double T = 20;  // in degree Celsius
    double P;
    double height;
    void start();
    void read();

  private:
    SFE_BMP180 pressure;
    bool restartTemperature = true;
    char statusPressure, statusTemperature;
    uint32_t t0, t1;
    bool restartPressure = true;
    double baseline = 1013.2;  // in mbar
};

#endif
