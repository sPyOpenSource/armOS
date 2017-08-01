/*
  Sensors.cpp - Library for Drone.
  Created by X. Wang, Januray 2, 2017.
  Released into the public domain.
*/

#include "Arduino.h"
#include "Sensors.h"

void Sensors::read()
{
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
// Initialize the sensor (it is important to get calibration values stored on the device).
  if (pressure.begin())
    Serial.println("BMP180 init success");
  else
  {
    // Oops, something went wrong, this is usually a connection p,roblem,
    // see the comments at the top of this sketch for the proper connections.
    Serial.println("BMP180 init fail (disconnected?)\n\n");
    while(1); // Pause forever.
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
