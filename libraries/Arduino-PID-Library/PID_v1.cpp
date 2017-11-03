/**********************************************************************************************
 * Arduino PID Library - Version 1.1.1
 * by Brett Beauregard <br3ttb@gmail.com> brettbeauregard.com
 *
 * This Library is licensed under a GPLv3 License
 **********************************************************************************************/

#if ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include <PID_v1.h>

void PID::Reset(){
   ITerm = 0;
}
/* Compute() **********************************************************************
 *     This, as they say, is where the magic happens.  this function should be called
 *   every time "void loop()" executes.  the function will decide for itself whether a new
 *   pid Output needs to be computed.  returns true when the output is computed,
 *   false when nothing has been done.
 **********************************************************************************/
void PID::Compute(double dt, float v)
{
   /*Compute all the working error variables*/
   double input = *myInput;
   double error = *mySetpoint - input;

   ITerm += (ki * error) * dt;
   if(ITerm > outMax*0.25) ITerm= outMax*0.25;
   else if(ITerm < outMin*0.25) ITerm= outMin*0.25;
   double dInput = (input - lastInput);

   /*Compute PID Output*/
   //double output = kp * error + ITerm - kd * dInput / dt;
   double output = kp * error + ITerm - kd * v;
   if(output > outMax) output = outMax;
   else if(output < outMin) output = outMin;
   *myOutput = output;

   /*Remember some variables for next time*/
   lastInput = input;
}


/* SetTunings(...)*************************************************************
 * This function allows the controller's dynamic performance to be adjusted.
 * it's called automatically from the constructor, but tunings can also
 * be adjusted on the fly during normal operation
 ******************************************************************************/
void PID::SetTunings(double Kp, double Ki, double Kd)
{
  if (Kp < 0 || Ki < 0 || Kd < 0) return;

  dispKp = Kp; dispKi = Ki; dispKd = Kd;
  kp = Kp; ki = Ki; kd = Kd;
  if(controllerDirection ==REVERSE)
   {
      kp = (0 - kp);
      ki = (0 - ki);
      kd = (0 - kd);
   }
}

/* SetOutputLimits(...)****************************************************
 *     This function will be used far more often than SetInputLimits.  while
 *  the input to the controller will generally be in the 0-1023 range (which is
 *  the default already,)  the output will be a little different.  maybe they'll
 *  be doing a time window and will need 0-8000 or something.  or maybe they'll
 *  want to clamp it from 0-125.  who knows.  at any rate, that can all be done
 *  here.
 **************************************************************************/
void PID::SetOutputLimits(double Min, double Max)
{
   if(Min >= Max) return;
   outMin = Min;
   outMax = Max;
}

/* Init()****************************************************************
 *	does all the things that need to happen to ensure a bumpless transfer
 *  from manual to automatic mode.
 ******************************************************************************/
void PID::Init(double* Input, double* Output, double* Setpoint,
        double Kp, double Ki, double Kd, int ControllerDirection, double Min, double Max)
{
   myOutput = Output;
   myInput = Input;
   mySetpoint = Setpoint;

   PID::SetOutputLimits(Min, Max);	//default output limit corresponds to
								                    //the arduino pwm limits

   PID::SetControllerDirection(ControllerDirection);
   PID::SetTunings(Kp, Ki, Kd);
   ITerm = *myOutput;
   lastInput = *myInput;
   if(ITerm > outMax) ITerm = outMax;
   else if(ITerm < outMin) ITerm = outMin;
}

/* SetControllerDirection(...)*************************************************
 * The PID will either be connected to a DIRECT acting process (+Output leads
 * to +Input) or a REVERSE acting process(+Output leads to -Input.)  we need to
 * know which one, because otherwise we may increase the output when we should
 * be decreasing.  This is called from the constructor.
 ******************************************************************************/
void PID::SetControllerDirection(int Direction)
{
   if(Direction != controllerDirection)
   {
      kp = (0 - kp);
      ki = (0 - ki);
      kd = (0 - kd);
   }
   controllerDirection = Direction;
}

/* Status Funcions*************************************************************
 * Just because you set the Kp=-1 doesn't mean it actually happened.  these
 * functions query the internal state of the PID.  they're here for display
 * purposes.  this are the functions the PID Front-end uses for example
 ******************************************************************************/
double PID::GetKp(){ return  dispKp; }
double PID::GetKi(){ return  dispKi; }
double PID::GetKd(){ return  dispKd; }
int PID::GetDirection(){ return controllerDirection; }
