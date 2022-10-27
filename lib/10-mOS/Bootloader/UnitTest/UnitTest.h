#ifndef UnitTest_h
#define UnitTest_h

#include "mbed.h"

//!Library for unit testing
/*!
This is an unit test class
*/

class UnitTest{
    private:
        Timer unitTest;
        unsigned int testCount;
    
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
        void assertOn(int, int);
};

#endif
