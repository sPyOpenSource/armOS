/*
  main.cpp - Main loop for Arduino sketches
  Copyright (c) 2005-2013 Arduino Team.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#define ARDUINO_MAIN
#include "AIDrone.h"
#include "UnitTest.h"

/*
 * Cortex-M3 Systick IT handler
 */
/*
extern void SysTick_Handler( void )
{
  // Increment tick count each ms
  TimeTick_Increment() ;
}
*/

// Weak empty variant initialization function.
// May be redefined by variant files.
void initVariant() __attribute__((weak));
void initVariant() { }

void testVectorDouble(){
  UnitTest unitTest;
  unitTest.start();
  VectorDouble testVector;
  testVector.setValues(1, 1, 1);
  unitTest.assert(testVector.x, 1.0);
  unitTest.assert(testVector.y, 1.0);
  unitTest.assert(testVector.z, 1.0);
  unitTest.assert(testVector.length(), 1.0);
  Receiver testReceiver;
  unitTest.assert(testReceiver.getInput(0), (unsigned int)1000);
  unitTest.end();
}

Drone drone;

void setup(){
  testVectorDouble();
  drone.start();
}

void loop(){
  drone.read();
  drone.compute();
  drone.write();
}


/*
 * \brief Main entry point of Arduino application
 */
int main( void )
{
	// Initialize watchdog
	watchdogSetup();

	init();

	initVariant();

	delay(1);

#if defined(USBCON)
	USBDevice.attach();
#endif

	setup();

	for (;;)
	{
		loop();
		if (serialEventRun) serialEventRun();
	}

	return 0;
}

extern "C" void __cxa_pure_virtual() {while (true);}
