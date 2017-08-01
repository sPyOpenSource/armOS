
/*
  AIDrone.h - Library for Drone.
  Created by X. Wang, January 8, 2017.
  Released into the public domain.
*/
#ifndef AIDrone_h
#define AIDrone_h

#include "Arduino.h"
#include <Sensors.h>
#include <Receiver.h>
#include <Wire.h>
#include <LSM303.h>
#include <Adafruit_L3GD20.h>
#include <Kalman.h>          // Source: https://github.com/TKJElectronics/KalmanFilter
#include <PID_v1.h>
#include <Servo.h> 

template <class T>
class Vector { 
   public: 
      T x, y, z; 
      void setValues(T X, T Y, T Z){
          x = X;
          y = Y;
          z = Z;
      };
}; 

class VectorDouble: public Vector<double> {
  public:
    static void cross(const VectorDouble * a, const VectorDouble * b, VectorDouble * out){
    	out->x = (a->y * b->z) - (a->z * b->y);
  	out->y = (a->z * b->x) - (a->x * b->z);
  	out->z = (a->x * b->y) - (a->y * b->x);
    };

    double length() {
	return sqrt(x*x+y*y+z*z);
    }

    void normalize(){
        double l = length();
        if (l>0){
            x = x/l;
            y = y/l;
            z = z/l;
        }
    }
};

class Drone {
    private:
        Servo myServo[4];
        LSM303 compass;
        Adafruit_L3GD20 gyro;
        Sensors sensors;
	Receiver receiver;

	VectorDouble E,N,setpoint,output,U,Kp,Ki,Kd,offset,QAcc,QMag,QbiasAcc,QbiasMag,RAcc,RMag,Maximum,factor,m_offset;
	
	int out = 1;
	int startingPoint = 1400;
	int timeout = 100000;           // in us
	int wait = 300000;              // in us
	double theta = 0;               // in rad
	const double phi = M_PI/4;      // in rad
	double opticalX = 0;
	double opticalY = 0;
	double angle;
	uint32_t timer;
	bool isSenderOn;

	int motor[4] = {1000,1000,1000,1000};

	const byte motorPin[4] = {9,10,11,12};
	Vector<Kalman> kalAcc,kalMag;
        Vector<int> neutral;
        Vector<PID> myPID;

    public:      
        Drone();
        void read();
        void write();
        void compute();
        void start();
};
#endif
