/*
  AIDrone.cpp - Library for Drone.
  Created by X. Wang, January 2, 2017.
  Released into the public domain.
*/

#include "Arduino.h"
#include "AIDrone.h"

Drone::Drone(){
   setpoint.setValues(    0      ,    0      ,    0      );
   output.setValues  (    0      ,    0      ,    0      );
   Kp.setValues      (  300      ,  300      ,    2      ); // 340
   Ki.setValues      (   15      ,   15      ,    0      );
   Kd.setValues      (   20      ,   20      ,    0      ); // 93
   offset.setValues  (  300      ,   50.5    , 1750      );
   QAcc.setValues    (    0.1    ,    0.1    ,    0.1    );
   QMag.setValues    (    0.2    ,    0.2    ,    0.2    );
   QbiasAcc.setValues(    0.02   ,    0.02   ,    0.02   );
   QbiasMag.setValues(    0.002  ,    0.002  ,    0.002  );
   RAcc.setValues    (    0.1    ,    0.1    ,    0.1    );
   RMag.setValues    (    0.002  ,    0.002  ,    0.002  );
   Maximum.setValues (  200      ,  200      ,  200      );
   factor.setValues  ( 1000      , 1000      ,    2      ); 
   //factor.setValues  (    2      ,    2      ,    2      );
   m_offset.setValues(  -47      ,  -43.5    ,   14      );
   neutral.setValues ( 1480      , 1500      , 1500      );
   
   myServo[0].attach(motorPin[0]);
   myServo[1].attach(motorPin[1]);
   myServo[2].attach(motorPin[2]);
   myServo[3].attach(motorPin[3]);
   myServo[0].writeMicroseconds(motor[0]);
   myServo[1].writeMicroseconds(motor[1]);
   myServo[2].writeMicroseconds(motor[2]);
   myServo[3].writeMicroseconds(motor[3]);
   kalAcc.x.setParameters(QAcc.x, QbiasAcc.x, RAcc.x);
   kalAcc.y.setParameters(QAcc.y, QbiasAcc.y, RAcc.y);
   kalAcc.z.setParameters(QAcc.z, QbiasAcc.z, RAcc.z);
   kalMag.x.setParameters(QMag.x, QbiasMag.x, RMag.x);
   kalMag.y.setParameters(QMag.y, QbiasMag.y, RMag.y);
   kalMag.z.setParameters(QMag.z, QbiasMag.z, RMag.z);
}

void Drone::read(){
  compass.read();
  gyro.read();
  if(Serial.available()>0){
      switch (Serial.read()){
      case '0':
        out = Serial.parseInt();
        Serial.println("0:OK");
        break;
      case '1':
        Kp.x = Serial.parseFloat();
        Kp.y = Serial.parseFloat();
        Kp.z = Serial.parseFloat();
        Ki.x = Serial.parseFloat();
        Ki.y = Serial.parseFloat();
        Ki.z = Serial.parseFloat();
        Kd.x = Serial.parseFloat();
        Kd.y = Serial.parseFloat();
        Kd.z = Serial.parseFloat();
        myPID.x.SetTunings(Kp.x,Ki.x,Kd.x);
        myPID.y.SetTunings(Kp.y,Ki.y,Kd.y);
        myPID.z.SetTunings(Kp.z,Ki.z,Kd.z);
        Serial.println("0:OK");
        break;
      case '2':
        QAcc.x = Serial.parseFloat();
        QAcc.y = Serial.parseFloat();
        QAcc.z = Serial.parseFloat();
        QbiasAcc.x = Serial.parseFloat();
        QbiasAcc.y = Serial.parseFloat();
        QbiasAcc.z = Serial.parseFloat();
        RAcc.x = Serial.parseFloat();
        RAcc.y = Serial.parseFloat();
        RAcc.z = Serial.parseFloat();
        kalAcc.x.setParameters(QAcc.x, QbiasAcc.x, RAcc.x);
        kalAcc.y.setParameters(QAcc.y, QbiasAcc.y, RAcc.y);
        kalAcc.z.setParameters(QAcc.z, QbiasAcc.z, RAcc.z);
        Serial.println("0:OK");
        break;
      case '3':
        offset.x = Serial.parseFloat();
        offset.y = Serial.parseFloat();
        offset.z = Serial.parseFloat();
        Serial.println("0:OK");
        break;
      case '4':
        m_offset.x = Serial.parseFloat();
        m_offset.y = Serial.parseFloat();
        m_offset.z = Serial.parseFloat();
        Serial.println("0:OK");
        break;
      case '5':
        opticalX = Serial.parseFloat();
        opticalY = Serial.parseFloat();
        break;
      default:
        Serial.println("0:BAD");
        break;
      }   
   }
}
VectorDouble rate, mrate, magnet;
void Drone::compute(){
  uint32_t start = micros();
  double dt = (double)(start - timer) / 1000000; // Calculate delta time in seconds
  timer = start;
  U.setValues(compass.a.x,compass.a.y,compass.a.z);
  U.normalize();
  rate.setValues((U.y * gyro.data.z - U.z * gyro.data.y) * DEG_TO_RAD, (U.z * gyro.data.x - U.x * gyro.data.z) * DEG_TO_RAD, (U.x * gyro.data.y - U.y * gyro.data.x) * DEG_TO_RAD);
  U.setValues(
      kalAcc.x.get(U.x, (U.y * gyro.data.z - U.z * gyro.data.y) * DEG_TO_RAD, dt),
      kalAcc.y.get(U.y, (U.z * gyro.data.x - U.x * gyro.data.z) * DEG_TO_RAD, dt),
      kalAcc.z.get(U.z, (U.x * gyro.data.y - U.y * gyro.data.x) * DEG_TO_RAD, dt)
  );
  mrate.setValues((N.y * gyro.data.z - N.z * gyro.data.y) * DEG_TO_RAD, (N.z * gyro.data.x - N.x * gyro.data.z) * DEG_TO_RAD, (N.x * gyro.data.y - N.y * gyro.data.x) * DEG_TO_RAD);
  N.setValues(compass.m.x-offset.x,compass.m.y-offset.y,compass.m.z-offset.z);
  N.normalize();
  magnet.setValues(N.x, N.y, N.z);
  VectorDouble::cross(&magnet, &U, &E);
  E.normalize();
  VectorDouble::cross(&U, &E, &magnet);
  N.setValues(
      kalMag.x.get(N.x, (N.y * gyro.data.z - N.z * gyro.data.y) * DEG_TO_RAD, dt),
      kalMag.y.get(N.y, (N.z * gyro.data.x - N.x * gyro.data.z) * DEG_TO_RAD, dt),
      kalMag.z.get(N.z, (N.x * gyro.data.y - N.y * gyro.data.x) * DEG_TO_RAD, dt)
  );
  VectorDouble::cross(&N, &U, &E);
  theta = atan2(E.y,E.x)-angle;
  theta = atan2(sin(theta),cos(theta));
  //InputZ -= (input[3]-1500)*M_PI/500;   
  E.normalize();
  VectorDouble::cross(&U, &E, &N);
  myPID.x.Compute(dt,kalAcc.x.getRate());
  myPID.y.Compute(dt,kalAcc.y.getRate());
  //myPID.z.Compute(dt,-gyro.data.z);
}

void Drone::start(){
   Serial.begin(115200);
   //Serial.println("start");
   //turn the PID on
   myPID.x.Init(&U.x , &output.x, &setpoint.x, Kp.x, Ki.x, Kd.x, DIRECT, -Maximum.x, Maximum.x);
   myPID.y.Init(&U.y , &output.y, &setpoint.y, Kp.y, Ki.y, Kd.y, DIRECT, -Maximum.y, Maximum.y);
   myPID.z.Init(&theta , &output.z, &setpoint.z, Kp.z, Ki.z, Kd.z, DIRECT, -Maximum.z, Maximum.z);
   Wire.begin();
   sensors.start();
   compass.init();
   compass.enableDefault();
   // Try to initialise and warn if we couldn't detect the chip
   if (!gyro.begin(gyro.L3DS20_RANGE_250DPS))
   {
       Serial.println("Oops ... unable to initialize the L3GD20. Check your wiring!");
       while (1);
   }
   timer = micros();
   receiver.start(timer);
   while(micros()-timer<wait){
       Drone::read();
   }
   N.setValues(compass.m.x-offset.x,compass.m.y-offset.y,compass.m.z-offset.z);
   U.setValues(compass.a.x,compass.a.y,compass.a.z);
   N.normalize();
   U.normalize();
   VectorDouble::cross(&N, &U, &E);
   angle = atan2(E.y,E.x);
   //setpoint.x = U.x;
   //setpoint.y = U.y;
   kalAcc.x.set(U.x);
   kalAcc.y.set(U.y);
   kalAcc.z.set(U.z);
   kalMag.x.set(N.x);
   kalMag.y.set(N.y);
   kalMag.z.set(N.z);
}
double dq, dqb, dr;
void Drone::write(){
  if((micros()-receiver.getTime())<timeout && (isSenderOn || receiver.getInput(2)<startingPoint)){
     isSenderOn = true;
     setpoint.x = - (receiver.getInput(1)-neutral.x)/factor.x;
     setpoint.y =   (receiver.getInput(0)-neutral.y)/factor.y;
     //m_offset.x = - (receiver.getInput(1)-neutral.x)/factor.x-45;
     //m_offset.y =   (receiver.getInput(0)-neutral.y)/factor.y-53;
     m_offset.z = - (receiver.getInput(3)-neutral.z)/factor.z+15;
     output.x+=m_offset.x;
     output.y+=m_offset.y;
     output.z=m_offset.z;
     dq = (receiver.getInput(4)-1000)/10.0;
     dqb = (receiver.getInput(5)-1000)/3.0;
     //dr = (receiver.getInput(2)-1150)/1000.0;
     myPID.x.SetTunings(dqb,Ki.x,dq);
     myPID.y.SetTunings(dqb,Ki.y,dq);
     //myPID.z.SetTunings(dq,dqb,0);
     //kalAcc.x.setParameters(QAcc.x, dq, RAcc.x);
     //kalAcc.y.setParameters(QAcc.y, dq, RAcc.y);
     //kalAcc.z.setParameters(QAcc.z, dq, RAcc.z);
     //kalMag.x.setParameters(dq, QbiasMag.x, dqb);
     //kalMag.y.setParameters(dq, QbiasMag.x, dqb);
     //kalMag.z.setParameters(dq, QbiasMag.x, dqb);
     motor[0] =   output.y * pow(sin(phi),2) + output.x * pow(cos(phi),2) + receiver.getInput(2) + output.z - opticalX - opticalY;
     motor[1] = - output.y * pow(cos(phi),2) + output.x * pow(sin(phi),2) + receiver.getInput(2) - output.z + opticalX - opticalY;
     motor[2] = - output.y * pow(sin(phi),2) - output.x * pow(cos(phi),2) + receiver.getInput(2) + output.z + opticalX + opticalY;
     motor[3] =   output.y * pow(cos(phi),2) - output.x * pow(sin(phi),2) + receiver.getInput(2) - output.z - opticalX + opticalY;
     myServo[0].writeMicroseconds(motor[0]);
     myServo[1].writeMicroseconds(motor[1]);
     myServo[2].writeMicroseconds(motor[2]);
     myServo[3].writeMicroseconds(motor[3]);
  } else {
     isSenderOn = false;
     myServo[0].writeMicroseconds(1000);
     myServo[1].writeMicroseconds(1000);
     myServo[2].writeMicroseconds(1000);
     myServo[3].writeMicroseconds(1000);
     myPID.x.Reset();
     myPID.y.Reset();
     myPID.z.Reset();
     angle = atan2(E.y,E.x);  
  }  
  double l = sqrt(pow(compass.a.x,2)+pow(compass.a.y,2)+pow(compass.a.z,2));

  switch(out){
     case 1:
         //Serial.print("1:");
         Serial.print(U.x);
         Serial.print(",");
         Serial.print(setpoint.x);
         Serial.print(",");
         Serial.print(output.x);
         Serial.print(",");
         Serial.print(U.y);
         Serial.print(",");
         Serial.print(setpoint.y);
         Serial.print(",");
         Serial.print(output.y);
         Serial.print(",");
         Serial.print(theta);
         Serial.print(",");
         Serial.println(output.z);
         break;
     case 2:
         //Serial.print("2:");
         Serial.print(U.x);
         Serial.print(",");
         Serial.print(U.y);
         Serial.print(",");
         Serial.println(U.z);
         /*Serial.print(",");
         Serial.print(compass.a.x/l);
         Serial.print(",");
         Serial.print(compass.a.y/l);
         Serial.print(",");
         Serial.println(compass.a.z/l);*/
         break;
     case 3:
         //Serial.print("3:");
         Serial.print(N.x);
         Serial.print(",");
         Serial.print(N.y);
         Serial.print(",");
         Serial.println(N.z);
         /*Serial.print(",");
         Serial.print(magnet.x);
         Serial.print(",");
         Serial.print(magnet.y);
         Serial.print(",");
         Serial.println(magnet.z);*/
         break;
     case 4:
         Serial.print("4:");
         Serial.print(N.x);
         Serial.print(",");
         Serial.print(E.x);
         Serial.print(",");
         Serial.print(U.x);
         Serial.print(",");
         Serial.print(N.z);
         Serial.print(",");
         Serial.print(E.z);
         Serial.print(",");
         Serial.println(U.z);
         break;
     case 5:
         //Serial.print("5:");
         Serial.print(motor[0]);
         Serial.print(",");
         Serial.print(motor[1]);
         Serial.print(",");
         Serial.print(motor[2]);
         Serial.print(",");
         Serial.println(motor[3]);
         break;
     case 6:
         //Serial.print("6:");
         Serial.print(compass.a.x);
         Serial.print(",");
         Serial.print(compass.a.y);
         Serial.print(",");
         Serial.println(compass.a.z);
         break;
     case 7:
         Serial.print("7:");
         Serial.print(compass.m.x);
         Serial.print(",");
         Serial.print(compass.m.y);
         Serial.print(",");
         Serial.println(compass.m.z);
         break;
     case 8:
     	 Serial.print("8:");
     	 Serial.print(m_offset.x);
     	 Serial.print(",");
     	 Serial.print(m_offset.y);
     	 Serial.print(",");
     	 Serial.println(m_offset.z);
     	 break;
     case 9:
     	 //Serial.print("9:");
     	 Serial.print(receiver.getInput(0));
     	 Serial.print(",");
     	 Serial.print(receiver.getInput(1));
     	 Serial.print(",");
     	 Serial.print(receiver.getInput(2));
     	 Serial.print(",");
     	 Serial.print(receiver.getInput(3));
     	 Serial.print(",");
     	 Serial.print(receiver.getInput(4));
     	 Serial.print(",");
     	 Serial.println(receiver.getInput(5));
     	 break;
     case 10:
     	 //Serial.print("10:");
     	 Serial.print(gyro.data.x);
     	 Serial.print(",");
     	 Serial.print(gyro.data.y);
     	 Serial.print(",");
     	 Serial.println(gyro.data.z);
     	 break;
     case 11:
         Serial.print("11:");
         Serial.print(dq*1000);
         Serial.print(",");
         Serial.print(dqb*1000);
         Serial.print(",");
         Serial.println(dr*1000);
         break;
     case 12:
         Serial.print(mrate.x);
         Serial.print(",");
         Serial.print(mrate.y);
         Serial.print(",");
         Serial.print(mrate.z);
         Serial.print(",");
         Serial.print(kalMag.x.getRate());
         Serial.print(",");
         Serial.print(kalMag.y.getRate());
         Serial.print(",");
         Serial.println(kalMag.z.getRate());
         break;
    }
}
