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
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SD.h>
#include <gui/desktop.h>
#include <gui/window.h>

#define GRAPHICSMODE

using namespace myos::gui;

const int TFT_DC = 15;
const int TFT_CS = 5;
const int SD_CS  = 4;  // Chip select line for SD card

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, 0/*no reset*/);

int i = 0;
void drawtext(const char *text, uint16_t color) {
  tft.setCursor(0, i * 10);
  i++;
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}

uint16_t read16(File f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

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

void testUnitTest(){
  UnitTest unitTest;
  unitTest.start();
  VectorDouble testVector;
  testVector.setValues(0, 3, 4);
  unitTest.assert(testVector.x, 0.0);
  unitTest.assert(testVector.y, 3.0);
  unitTest.assert(testVector.z, 4.0);
  unitTest.assert(testVector.length(), 5.0);
  Receiver testReceiver;
  unitTest.assert(testReceiver.getInput(0), (unsigned int)1000);
  Kalman testKalman;
  testKalman.setParameters(1, 1, 1);
  testKalman.set(1);
  //unitTest.assert(testKalman.get(1, 1, 1), 1.0);
  //unitTest.assert(testKalman.getRate(), 1.0);
  PID testPID;
  double a = 1, b, c = 1;
  testPID.Init(&a, &b, &c, 1, 1, 1, 1, 1, 1);
  testPID.Compute(1, 1);
  //unitTest.assert(b,-3.0);
  unitTest.end();
}

Drone drone;

#ifdef GRAPHICSMODE
    Desktop desktop(128, 160, 0x00, 0x00, 0xA8);
#endif

void setup(){
    Serial.begin(115200);
    tft.initR(INITR_BLACKTAB);
    tft.fillScreen(ST77XX_BLACK);
    Serial.print("Initializing SD...");
    drawtext("Initializing SD...", ST77XX_WHITE);
    if (!SD.begin(SD_CS)) {
        Serial.println("failed!");
        drawtext("failed!", ST77XX_WHITE);
    } else {
        Serial.println("OK!");
        drawtext("OK!", ST77XX_WHITE);
    }
    testUnitTest();
    desktop.Draw(&tft);
    //tft.fillRect(10, 12, 10, 10, ST77XX_WHITE);
    drone.init();

    printf("Initializing Hardware, Stage 1\n");

        #ifdef GRAPHICSMODE
            //KeyboardDriver keyboard(&interrupts, &desktop);
        #else
            PrintfKeyboardEventHandler kbhandler;
            KeyboardDriver keyboard(&interrupts, &kbhandler);
        #endif

        #ifdef GRAPHICSMODE
            //MouseDriver mouse(&interrupts, &desktop);
        #else
            MouseToConsole mousehandler;
            MouseDriver mouse(&interrupts, &mousehandler);
        #endif

        #ifdef GRAPHICSMODE
            //VideoGraphicsArray vga;
        #endif

    printf("Initializing Hardware, Stage 2\n");

    printf("Initializing Hardware, Stage 3\n");

    #ifdef GRAPHICSMODE
        //vga.SetMode(320, 200, 8);
        Window win1(&desktop, 10, 10, 20, 20, 0x00, 0xA8, 0x00);
        desktop.AddChild(&win1);
        Window win2(&desktop, 40, 15, 30, 30, 0x00, 0xA8, 0x00);
        desktop.AddChild(&win2);
    #endif
}

void loop(){
  #ifdef GRAPHICSMODE
            desktop.Draw(&tft);
  #endif
  drone.compute();
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
