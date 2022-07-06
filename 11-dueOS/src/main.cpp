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
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include "AIDrone.h"
#include <gui/desktop.h>
#include <gui/window.h>
#include <MouseController.h>
#include <KeyboardController.h>
#include <SD.h>
#include <PS2Mouse.h>
#include "UnitTest.h"

#define MOUSE_DATA 7
#define MOUSE_CLOCK 6
#define GRAPHICSMODE

#define Z_STEP_PIN         46
#define Z_DIR_PIN          48
#define Z_ENABLE_PIN       62

#define E_STEP_PIN         26
#define E_DIR_PIN          28
#define E_ENABLE_PIN       24

#define STEPS              2000

#define HEATER_0_PIN       9
#define TEMP_0_PIN         13   // ANALOG NUMBERING
#define TEMP_1_PIN         14   // ANALOG NUMBERING

using namespace myos::gui;

const int TFT_A0 = 15;
const int TFT_CS = 5;
const int SD_CS  = 4;  // Chip select line for SD card
const int TRIG   = 26;
const int ECHO   = 28;

boolean down;
String readString;

float x0 = 0;
float targetTemp = 0;
int n = 0;
int k = 1;
int m = 0;

// Initialize USB Controller
USBHost usb;

// Attach mouse controller to USB
//MouseController mouse(usb);

// Attach keyboard controller to USB
KeyboardController keyboard(usb);

// variables for mouse button states
boolean leftButton = false;
boolean middleButton = false;
boolean rightButton = false;

// This function intercepts mouse movements while a button is pressed
/*void mouseDragged() {
  Serial.print("DRAG: ");
  Serial.print(mouse.getXChange());
  Serial.print(", ");
  Serial.println(mouse.getYChange());
}

// This function intercepts mouse button press
void mousePressed() {
  Serial.print("Pressed: ");
  if (mouse.getButton(LEFT_BUTTON)){
    Serial.print("L");
    leftButton = true;
  }
  if (mouse.getButton(MIDDLE_BUTTON)){
    Serial.print("M");
    middleButton = true;
  }
  if (mouse.getButton(RIGHT_BUTTON)){
    Serial.print("R");
    Serial.println();
    rightButton = true;
  }
}

// This function intercepts mouse button release
void mouseReleased() {
  Serial.print("Released: ");
  if (!mouse.getButton(LEFT_BUTTON) && leftButton == true) {
    Serial.print("L");
    leftButton = false;
  }
  if (!mouse.getButton(MIDDLE_BUTTON) && middleButton == true) {
    Serial.print("M");
    middleButton = false;
  }
  if (!mouse.getButton(RIGHT_BUTTON) && rightButton == true) {
    Serial.print("R");
    rightButton = false;
  }
  Serial.println();
}*/

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_A0, -1);

int line = 0;
int position = 0;
void print(const char *text, uint16_t color) {
  Serial.println(text);
  tft.setCursor(0, line * 10);
  line++;
  if (line == 16){
      tft.fillScreen(ST77XX_BLACK);
      line = 0;
  }
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}

void print(const char text, uint16_t color) {
  Serial.print(text);
  tft.setCursor(position * 7, line * 10);
  position++;
  if(position >= 18){
    line++;
    position = 0;
  }
  if (line == 16){
      tft.fillScreen(ST77XX_BLACK);
      line = 0;
  }
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}

void printKey() {
  // getOemKey() returns the OEM-code associated with the key
  Serial.print(" key:");
  Serial.print(keyboard.getOemKey());

  // getModifiers() returns a bits field with the modifiers-keys
  int mod = keyboard.getModifiers();
  Serial.print(" mod:");
  Serial.print(mod);

  Serial.print(" => ");
  char key = keyboard.getKey();
  if (mod & LeftCtrl) {
    Serial.print("L-Ctrl ");
  }
  if (mod & LeftShift) {
    Serial.print("L-Shift ");
    key += 26;
  }
  if (mod & Alt) {
    Serial.print("Alt ");
  }
  if (mod & LeftCmd) {
    Serial.print("L-Cmd ");
  }
  if (mod & RightCtrl) {
    Serial.print("R-Ctrl ");
  }
  if (mod & RightShift) {
    Serial.print("R-Shift ");
    key += 26;
  }
  if (mod & AltGr) {
    Serial.print("AltGr ");
  }
  if (mod & RightCmd) {
    Serial.print("R-Cmd ");
  }

  // getKey() returns the ASCII translation of OEM key
  // combined with modifiers.
  print(key, ST77XX_WHITE);
  Serial1.print(key);
}

// This function intercepts key press
void keyPressed() {
  Serial.print("Pressed:  ");
  printKey();
}

// This function intercepts key release
void keyReleased() {
  Serial.print("Released: ");
  //printKey();
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
  PID testPID;
  double a = 1, b, c = 1;
  testPID.Init(&a, &b, &c, 1, 1, 1, 1, 1, 1);
  testPID.Compute(1, 1);
  unitTest.end();
}

Drone drone;

#ifdef GRAPHICSMODE
    Desktop desktop(128, 160, 0x00, 0x00, 0xA8);
#endif

    PS2Mouse pad(MOUSE_CLOCK, MOUSE_DATA, STREAM);

// This function intercepts mouse movements
/*void mouseMoved() {
  Serial.print("Move: ");
  desktop.OnMouseMove(mouse.getXChange(), mouse.getYChange());
}*/

// Anything over 400 cm (23200 us pulse) is "out of range"
const unsigned int MAX_DIST = 23200;

String getValue(String data, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == ' ' || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void setup(){
    Serial.begin(115200);
    Serial1.begin(115200);
    Serial2.begin(57600);
    while(true){
      if (Serial1.available()) {      // If anything comes in Serial (USB),
        Serial.write(Serial1.read());   // read it and send it out Serial1 (pins 0 & 1)
      }

      if (Serial.available()) {     // If anything comes in Serial1 (pins 0 & 1)]
          char c = Serial.read();
          Serial1.write(c);   // read it and send it out Serial (USB)
          //Serial.write(c);
      }
    }
    tft.initR(INITR_BLACKTAB);
    tft.fillScreen(ST77XX_BLACK);

    pinMode(TRIG, OUTPUT);
    digitalWrite(TRIG, LOW);

    pinMode(Z_STEP_PIN, OUTPUT);
    pinMode(Z_DIR_PIN, OUTPUT);
    pinMode(Z_ENABLE_PIN, OUTPUT);

    pinMode(E_STEP_PIN, OUTPUT);
    pinMode(E_DIR_PIN, OUTPUT);
    pinMode(E_ENABLE_PIN, OUTPUT);

    pinMode(HEATER_0_PIN, OUTPUT);

    digitalWrite(Z_ENABLE_PIN, LOW);
    digitalWrite(E_ENABLE_PIN, LOW);

    digitalWrite(Z_DIR_PIN, LOW);
    digitalWrite(E_DIR_PIN, HIGH);

    digitalWrite(HEATER_0_PIN, LOW);

    testUnitTest();

    print("Initializing SD...", ST77XX_WHITE);
    if (!SD.begin(SD_CS)) {
        print("failed!", ST77XX_WHITE);
    } else {
        print("OK!", ST77XX_WHITE);
    }

    printf("Initializing Hardware, Stage 1\r\n");

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

    //pad.initialize();
    printf("Initializing Hardware, Stage 2\r\n");
    //drone.init();

    printf("Initializing Hardware, Stage 3\r\n");

    #ifdef GRAPHICSMODE
        Window win1(&desktop, 10, 10, 20, 20, 0xFF, 0x00, 0x00);
        desktop.AddChild(&win1);
        Window win2(&desktop, 40, 15, 30, 30, 0x00, 0xA8, 0x00);
        desktop.AddChild(&win2);
    #endif

    while(true){
      if (serialEventRun) serialEventRun();
      loop();
    }
}

void loop(){
  // Process USB tasks
  usb.Task();
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  while ( digitalRead(ECHO) == 0 );

  // Measure how long the echo pin was held high (pulse width)
  // Note: the micros() counter will overflow after ~70 min
  int t1 = micros();
  while ( digitalRead(ECHO) == 1);
  int t2 = micros();
  int pulse_width = t2 - t1;

  // Calculate distance in centimeters and inches. The constants
  // are found in the datasheet, and calculated from the assumed speed
  //of sound in air at sea level (~340 m/s).
  float cm = pulse_width / 58.0;
  Serial.println(cm);
  //int16_t data[3];
  //pad.report(data);
  //Serial.print(data[0]); // Status Byte
  //Serial.print(":");
  //Serial.print(data[1]); // X Movement Data
  //Serial.print(",");
  //Serial.print(data[2]); // Y Movement Data
  //Serial.println();
  #ifdef GRAPHICSMODE
        //desktop.Draw(&tft);
  #endif
  //drone.compute();
  delay(33);
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

	return 0;
}

extern "C" void __cxa_pure_virtual() {while (true);}
