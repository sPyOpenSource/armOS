/*
  AIDrone.cpp - Library for Drone.
  Created by X. Wang, January 2, 2017.
  Released into the public domain.
*/

#include "AIDrone.h"

Drone::Drone(){
   setpoint.setValues(    0      ,    0      ,    0      );
   output.setValues  (    0      ,    0      ,    0      );
   Kp.setValues      (  300      ,  300      ,    2      );
   Ki.setValues      (   15      ,   15      ,    0      );
   Kd.setValues      (   20      ,   20      ,    0      );
   offset.setValues  (  300      ,   50.5    , 1750      );
   QAcc.setValues    (    0.1    ,    0.1    ,    0.1    );
   QMag.setValues    (    0.2    ,    0.2    ,    0.2    );
   QbiasAcc.setValues(    0.02   ,    0.02   ,    0.02   );
   QbiasMag.setValues(    0.002  ,    0.002  ,    0.002  );
   RAcc.setValues    (    0.1    ,    0.1    ,    0.1    );
   RMag.setValues    (    0.002  ,    0.002  ,    0.002  );
   Maximum.setValues (  200      ,  200      ,  200      );
   factor.setValues  ( 1000      , 1000      ,    2      );
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
   kalAcc.setParameters(&QAcc, &QbiasAcc, &RAcc);
   kalMag.setParameters(&QMag, &QbiasMag, &RMag);
}

void Drone::read(){
  lsm303.read();
  gyro.read();
  if(Serial.available() > 0){
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
        myPID.x.SetTunings(Kp.x, Ki.x, Kd.x);
        myPID.y.SetTunings(Kp.y, Ki.y, Kd.y);
        myPID.z.SetTunings(Kp.z, Ki.z, Kd.z);
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
        kalAcc.setParameters(&QAcc, &QbiasAcc, &RAcc);
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

void Drone::compute(){
  uint32_t start = micros();
  double dt = (double)(start - timer) / 1000000; // Calculate delta time in seconds
  timer = start;

  lsm303.a.normalize();
  VectorDouble::cross(&lsm303.a, &gyro.data, &arate);
  kalAcc.update(&lsm303.a, &arate, dt),

  lsm303.m.minus(&offset);
  lsm303.m.normalize();
  VectorDouble::cross(&lsm303.m, &gyro.data, &mrate);
  kalMag.update(&lsm303.m, &mrate, dt),

  VectorDouble::cross(&lsm303.m, &lsm303.a, &E);
  theta = atan2(E.y, E.x) - angle;
  theta = atan2(sin(theta), cos(theta));
  E.normalize();
  VectorDouble::cross(&lsm303.a, &E, &lsm303.m);

  myPID.x.Compute(dt, kalAcc.x.getRate());
  myPID.y.Compute(dt, kalAcc.y.getRate());
}

void Drone::start(){
   //turn the PID on
   myPID.x.Init(&lsm303.a.x , &output.x, &setpoint.x, Kp.x, Ki.x, Kd.x, DIRECT, -Maximum.x, Maximum.x);
   myPID.y.Init(&lsm303.a.y , &output.y, &setpoint.y, Kp.y, Ki.y, Kd.y, DIRECT, -Maximum.y, Maximum.y);
   myPID.z.Init(&theta , &output.z, &setpoint.z, Kp.z, Ki.z, Kd.z, DIRECT, -Maximum.z, Maximum.z);
   Wire.begin();
   lsm303.init();
   lsm303.enableDefault();

   // Try to initialise and warn if we couldn't detect the chip
   if (!gyro.begin(gyro.L3DS20_RANGE_250DPS))
   {
       Serial.println("Oops ... unable to initialize the L3GD20. Check your wiring!");
       while (true){
         delay(60);
       }
   }
   timer = micros();
   receiver.start(timer);
   while(micros() - timer < wait){
       Drone::read();
   }
   lsm303.m.minus(&offset);
   lsm303.m.normalize();
   lsm303.a.normalize();
   VectorDouble::cross(&lsm303.m, &lsm303.a, &E);
   angle = atan2(E.y, E.x);
   kalAcc.set(&lsm303.a);
   kalMag.set(&lsm303.m);
}

double dq, dqb, dr;
void Drone::write(){
  if((micros() - receiver.getTime()) < timeout && (isSenderOn || receiver.getInput(2) < startingPoint)){
     isSenderOn = true;
     setpoint.x = - (receiver.getInput(1) - neutral.x) / factor.x;
     setpoint.y =   (receiver.getInput(0) - neutral.y) / factor.y;
     m_offset.z = - (receiver.getInput(3) - neutral.z) / factor.z + 15;
     output.x += m_offset.x;
     output.y += m_offset.y;
     output.z = m_offset.z;
     dq = (receiver.getInput(4) - 1000) / 10.0;
     dqb = (receiver.getInput(5) - 1000) / 3.0;
     myPID.x.SetTunings(dqb, Ki.x, dq);
     myPID.y.SetTunings(dqb, Ki.y, dq);
     motor[0] =   output.y * pow(sin(phi), 2) + output.x * pow(cos(phi), 2) + receiver.getInput(2) + output.z - opticalX - opticalY;
     motor[1] = - output.y * pow(cos(phi), 2) + output.x * pow(sin(phi), 2) + receiver.getInput(2) - output.z + opticalX - opticalY;
     motor[2] = - output.y * pow(sin(phi), 2) - output.x * pow(cos(phi), 2) + receiver.getInput(2) + output.z + opticalX + opticalY;
     motor[3] =   output.y * pow(cos(phi), 2) - output.x * pow(sin(phi), 2) + receiver.getInput(2) - output.z - opticalX + opticalY;
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
     angle = atan2(E.y, E.x);
  }

  switch(out){
     case 5:
         Serial.print(motor[0]);
         Serial.print(",");
         Serial.print(motor[1]);
         Serial.print(",");
         Serial.print(motor[2]);
         Serial.print(",");
         Serial.println(motor[3]);
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
      default:
         break;
    }
}
