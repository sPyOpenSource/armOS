
/*
  Receiver.h - Library for Drone.
  Created by X. Wang, January 8, 2017.
  Released into the public domain.
*/

#ifndef Receiver_h
#define Receiver_h

#include "Arduino.h"

class Receiver {
    public:
       unsigned int getInput(unsigned char);
       void start(uint32_t);
       uint32_t getTime();
};

#endif
