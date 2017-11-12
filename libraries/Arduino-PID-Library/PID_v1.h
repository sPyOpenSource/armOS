#ifndef PID_v1_h
#define PID_v1_h
#define LIBRARY_VERSION	1.1.1

#if ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

class PID
{
  public:
    //Constants used in some of the functions below
    #define DIRECT  0
    #define REVERSE 1

    //commonly used functions **************************************************************************

    void Reset();
    void Compute(double, double);         // * performs the PID calculation.  it should be
                                          //   called every time loop() cycles. ON/OFF and
                                          //   calculation frequency can be set using SetMode
                                          //   SetSampleTime respectively

    void Init(double*, double*, double*,  // * constructor.  links the PID to the Input, Output, and
        double, double, double, int, double, double);

    //available but not commonly used functions ********************************************************
    void SetTunings(double, double,       // * While most users will set the tunings once in the
                    double);         	    //   constructor, this function gives the user the option
                                          //   of changing tunings during runtime for Adaptive control
    void SetControllerDirection(int);     // * Sets the Direction, or "Action" of the controller. DIRECT
                              					  //   means the output will increase when error is positive. REVERSE
                              					  //   means the opposite.  it's very unlikely that this will be needed
                              					  //   once it is set in the constructor.

  private:
  	void SetOutputLimits(double, double);   //clamps the output to a specific range. 0-255 by default, but
                      										  //it's likely the user will want to change this depending on
                      										  //the application

  	double kp;                  // * (P)roportional Tuning Parameter
    double ki;                  // * (I)ntegral Tuning Parameter
    double kd;                  // * (D)erivative Tuning Parameter

  	int controllerDirection;

    double *myInput;              // * Pointers to the Input, Output, and Setpoint variables
    double *myOutput;             //   This creates a hard link between the variables and the
    double *mySetpoint;           //   PID, freeing the user from having to constantly tell us
                                  //   what these values are.  with pointers we'll just know.

  	double ITerm, lastInput, outMin, outMax;
};
#endif
