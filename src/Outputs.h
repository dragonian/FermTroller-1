#ifndef OUTPUTS_H
#define OUTPUTS_H


#include <Arduino.h>
#include <pin.h>

void pinInit();

void resetOutputs();

void processOutputs();

void updateValves();

unsigned long computeValveBits();

boolean vlvConfigIsActive(byte profile);

boolean isAlarmAllZones();

void updateAlarm();

void setBuzzer(boolean alarmON);


#endif
