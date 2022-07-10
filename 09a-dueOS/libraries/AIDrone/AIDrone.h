
/*
  AIDrone.h - Library for Drone.
  Created by X. Wang, January 8, 2017.
  Released into the public domain.
*/
#ifndef AIDrone_h
#define AIDrone_h

#include <Receiver.h>
#include <LSM303.h>
#include <Adafruit_L3GD20.h>
#include <PID_v1.h>
#include <Servo.h>

class Drone {
    private:
        Servo myServo[4];
        LSM303 lsm303;
        Adafruit_L3GD20 gyro;
      	Receiver receiver;
      	VectorDouble E, setpoint, output, Kp, Ki, Kd, offset, QAcc, QMag, QbiasAcc, QbiasMag, RAcc, RMag, Maximum, factor, m_offset, arate, mrate;
        double angle;
        uint32_t timer;
        bool isSenderOn;
        VectorKalman kalAcc, kalMag;
        Vector<int> neutral;
        Vector<PID> myPID;

      	int out = 8;
      	int startingPoint = 1400;
      	int timeout = 100000;           // in us
      	int wait = 300000;              // in us
      	double theta = 0;               // in rad
      	const double phi = M_PI / 4;    // in rad
      	double opticalX = 0;
      	double opticalY = 0;
      	int motor[4] = {1000, 1000, 1000, 1000};
      	const byte motorPin[4] = {9, 10, 11, 12};
        const byte rightPWM     = 5;
        const byte leftPWM      = 23;
        const byte leftInL      = 27;
        const byte leftInH      = 26;
        const byte rightInL     = 2;
        const byte rightInH     = 4;
        const byte rightSensor  = 19;
        const byte leftSensor   = 13;
        const byte centerSensor = 12;
        void read();
        void write();

    public:
        Drone();
        void compute();
        void init();
};
#endif
