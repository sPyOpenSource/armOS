#include "mbed.h"
#include "UnitTest.h"

extern void bootloader(void);
DigitalOut _led1(PTD3);
DigitalOut _led4(PTD6);

int main() 
{   
    _led1 = 0;
    _led4 = 1;
    UnitTest unitest;
    unitest.assertOn(_led4, 1);
    bootloader();
    
    mbed_start_application(0xb400);
}