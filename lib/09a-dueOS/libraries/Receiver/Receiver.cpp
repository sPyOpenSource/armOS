/*
  Receiver.cpp - Library for Drone.
  Created by X. Wang, Januray 2, 2017.
  Released into the public domain.
*/

#include "Receiver.h"

const byte interruptPins[4] = {49, 46, 50, 52};
volatile unsigned int inputs[6] = {1000, 1000, 1000, 1000, 1000, 1000};
volatile uint32_t times[2];

void end() {
  times[0] = micros();
  int temp = times[0] - times[1];
  if (temp < 2000 && temp > 1000){
  	inputs[5] = temp;
  }
  //detachInterrupt(digitalPinToInterrupt(interruptPins[3]));
}

void stage6() {
  times[1] = micros();
  int temp = times[1] - times[0];
  if (temp < 2000 && temp > 1000){
  	inputs[4] = temp;
  }
}

void stage5() {
  times[0] = micros();
  int temp = times[0] - times[1];
  if (temp < 2000 && temp > 1000){
  	inputs[3] = temp;
  }
  //detachInterrupt(digitalPinToInterrupt(interruptPins[2]));
}

void stage4() {
  times[1] = micros();
  int temp = times[1] - times[0];
  if (temp < 2000 && temp > 1000){
  	inputs[2] = temp;
  }
}

void stage3() {
  times[0] = micros();
  int temp = times[0] - times[1];
  if (temp < 2000 && temp > 1000){
  	inputs[1] = temp;
  }
  //detachInterrupt(digitalPinToInterrupt(interruptPins[1]));
}

void stage2() {
  times[1] = micros();
  int temp = times[1] - times[0];
  if (temp < 2000 && temp > 1000){
  	inputs[0] = temp;
  }
}

void stage1() {
  times[0] = micros();
}

void Receiver::start(uint32_t t){
   times[0] = t;
   pinMode(interruptPins[0], INPUT_PULLUP);
   pinMode(interruptPins[1], INPUT_PULLUP);
   pinMode(interruptPins[2], INPUT_PULLUP);
   pinMode(interruptPins[3], INPUT_PULLUP);
   attachInterrupt(digitalPinToInterrupt(interruptPins[0]), stage1, RISING);
   attachInterrupt(digitalPinToInterrupt(interruptPins[1]), stage2, RISING);
   attachInterrupt(digitalPinToInterrupt(interruptPins[1]), stage3, FALLING);
   attachInterrupt(digitalPinToInterrupt(interruptPins[2]), stage5, FALLING);
   attachInterrupt(digitalPinToInterrupt(interruptPins[2]), stage4, RISING);
   attachInterrupt(digitalPinToInterrupt(interruptPins[3]), stage6, RISING);
   attachInterrupt(digitalPinToInterrupt(interruptPins[3]), end, FALLING);
}

unsigned int Receiver::getInput(unsigned char channel){
  return inputs[channel];
}

uint32_t Receiver::getTime(){
  return times[0] + 20000;
}
