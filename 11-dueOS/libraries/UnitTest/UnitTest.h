#ifndef UnitTest_h
#define UnitTest_h

#include "Arduino.h"

//!Library for unit testing
/*!
This is an unit test class
*/
class UnitTest
{
    private:
        unsigned int testCount;
        unsigned long time;

    public:
        /**
        *   This is the start point of the unit test
        */
        void start();

        /**
        *   This is the end point of the unit test
        */
        void end();

        /**
        *   This is a test function
        */
        void assert(int, int);

        void assert(unsigned int, unsigned int);

        void assert(double, double);

        static void stop(){
          while(true){
            delay(60);
          }
        }
};

#endif
