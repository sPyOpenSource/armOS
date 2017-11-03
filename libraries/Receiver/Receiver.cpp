/*
  Receiver.cpp - Library for Drone.
  Created by X. Wang, Januray 2, 2017.
  Released into the public domain.
*/

#include "Receiver.h"

const byte interruptPins[4] = {53, 51, 49, 47};
volatile unsigned int inputs[6] = {1000, 1000, 1000, 1000, 1000, 1000};
volatile uint32_t times[7];

void end() {
  times[6] = micros();
  int temp = times[6] - times[5];
  if (temp < 2000 && temp > 1000)
  	inputs[5] = temp;
  detachInterrupt(digitalPinToInterrupt(interruptPins[3]));
}

void stage6() {
  times[5] = micros();
  int temp = times[5] - times[4];
  if (temp < 2000 && temp > 1000)
  	inputs[4] = temp;
  attachInterrupt(digitalPinToInterrupt(interruptPins[3]), end, FALLING);
}

void stage5() {
  times[4] = micros();
  int temp = times[4] - times[3];
  if (temp < 2000 && temp > 1000)
  	inputs[3] = temp;
  attachInterrupt(digitalPinToInterrupt(interruptPins[3]), stage6, RISING);
  detachInterrupt(digitalPinToInterrupt(interruptPins[2]));
}

void stage4() {
  times[3] = micros();
  int temp = times[3] - times[2];
  if (temp < 2000 && temp > 1000)
  	inputs[2] = temp;
  attachInterrupt(digitalPinToInterrupt(interruptPins[2]), stage5, FALLING);
}

void stage3() {
  times[2] = micros();
  int temp = times[2] - times[1];
  if (temp < 2000 && temp > 1000)
  	inputs[1] = temp;
  attachInterrupt(digitalPinToInterrupt(interruptPins[2]), stage4, RISING);
  detachInterrupt(digitalPinToInterrupt(interruptPins[1]));
}

void stage2() {
  times[1] = micros();
  int temp = times[1] - times[0];
  if (temp < 2000 && temp > 1000)
  	inputs[0] = temp;
  attachInterrupt(digitalPinToInterrupt(interruptPins[1]), stage3, FALLING);
}

void stage1() {
  times[0] = micros();
  attachInterrupt(digitalPinToInterrupt(interruptPins[1]), stage2, RISING);
}

void Receiver::start(uint32_t t){
   times[0] = t;
   pinMode(interruptPins[0], INPUT_PULLUP);
   pinMode(interruptPins[1], INPUT_PULLUP);
   pinMode(interruptPins[2], INPUT_PULLUP);
   pinMode(interruptPins[3], INPUT_PULLUP);
   attachInterrupt(digitalPinToInterrupt(interruptPins[0]), stage1, RISING);
}

unsigned int Receiver::getInput(unsigned char channel){
  return inputs[channel];
}

uint32_t Receiver::getTime(){
  return times[0] + 20000;
}
