/*
  Sensors.cpp - Library for Drone.
  Created by X. Wang, Januray 2, 2017.
  Released into the public domain.
*/

#include "Sensors.h"

void Sensors::read()
{
  lsm303.read();
  gyro.read();

  if(restartTemperature){
    statusTemperature = pressure.startTemperature();
    t0 = millis();
  }
  if (statusTemperature != 0)
  {
    restartTemperature = false;
    if (millis()-t0>=statusTemperature){
      statusTemperature = pressure.getTemperature(T);
      restartTemperature = true;
      if (statusTemperature == 0)
          Serial.println("error retrieving pressure measurement\n");
    }
  }
  else Serial.println("error starting temperature measurement\n");
  if(restartPressure){
     // Start a pressure measurement:
     // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
     // If request is successful, the number of ms to wait is returned.
     // If request is unsuccessful, 0 is returned.
     statusPressure = pressure.startPressure(3);
     t1 = millis();
  }
  if (statusPressure != 0)
  {
     restartPressure = false;
     if(millis()-t1>=statusPressure){
        statusPressure = pressure.getPressure(P,T);
        restartPressure = true;
        if (statusPressure == 0)
          Serial.println("error retrieving pressure measurement\n");
        else
          height = pressure.altitude(P,baseline);
     }
  }
  else Serial.println("error starting pressure measurement\n");
}

void Sensors::start(){
  Wire.begin();
  lsm303.init();
  lsm303.enableDefault();

  // Try to initialise and warn if we couldn't detect the chip
  if (!gyro.begin(gyro.L3DS20_RANGE_250DPS))
  {
      Serial.println("Oops ... unable to initialize the L3GD20. Check your wiring!");
      UnitTest::stop();
  }
  
  acc = lsm303.a;
  mag = lsm303.m;
  gyr = gyro.data;

  // Initialize the sensor (it is important to get calibration values stored on the device).
  if (!pressure.begin())
  {
    // Oops, something went wrong, this is usually a connection p,roblem,
    // see the comments at the top of this sketch for the proper connections.
    Serial.println("BMP180 init fail (disconnected?)\n\n");
    UnitTest::stop();
  }
  statusTemperature = pressure.startTemperature();
  if (statusTemperature != 0)
  {
    delay(statusTemperature);
    statusTemperature = pressure.getTemperature(T);
    if (statusTemperature == 0)
          Serial.println("error retrieving temperature measurement\n");
    else
    {
          Serial.print(T);
          Serial.println(" oC");
          // Start a pressure measurement:
          // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
          // If request is successful, the number of ms to wait is returned.
          // If request is unsuccessful, 0 is returned.
          statusPressure = pressure.startPressure(3);
          if (statusPressure != 0)
          {
              delay(statusPressure);
              statusPressure = pressure.getPressure(P,T);
              if (statusPressure == 0)
                Serial.println("error retrieving pressure measurement\n");
              else
                baseline = P;
          }
          else Serial.println("error starting pressure measurement\n");
      }
  }
  else Serial.println("error starting temperature measurement\n");

  Serial.print("baseline pressure: ");
  Serial.print(baseline);
  Serial.println("mbar");
}
