/*  
   Copyright (C) 2009, 2010 Matt Reba, Jeremiah Dillingham

    This file is part of BrewTroller.

    BrewTroller is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    BrewTroller is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with BrewTroller.  If not, see <http://www.gnu.org/licenses/>.


BrewTroller - Open Source Brewing Computer
Software Lead: Matt Reba (matt_AT_brewtroller_DOT_com)
Hardware Lead: Jeremiah Dillingham (jeremiah_AT_brewtroller_DOT_com)

Documentation, Forums and more information available at http://www.brewtroller.com
*/
#include "wiring_private.h"

#include "Config.h"
#include "Enum.h"
#include "HWProfile.h"
#include "PVOut.h"
#include "FermTroller.h"
#include "Outputs.h"
#include "Events.h"

// set what the PID cycle time should be based on how fast the temp sensors will respond
#if TS_ONEWIRE_RES == 12
  #define PID_CYCLE_TIME 750
#elif TS_ONEWIRE_RES == 11
  #define PID_CYCLE_TIME 375
#elif TS_ONEWIRE_RES == 10
  #define PID_CYCLE_TIME 188
#elif TS_ONEWIRE_RES == 9
  #define PID_CYCLE_TIME 94
#else
  // should not be this value, fail the compile
  #ERROR
#endif

void pinInit() {
  #ifdef HEARTBEAT
    hbPin.setup(HEARTBEAT_PIN, OUTPUT);
  #endif
  
  #ifdef DIGITAL_INPUTS
  #if DIGIN_COUNT > 0
    digInPin[0].setup(DIGIN1_PIN, INPUT);
  #endif
  #if DIGIN_COUNT > 1
    digInPin[1].setup(DIGIN2_PIN, INPUT);
  #endif
  #if DIGIN_COUNT > 2
    digInPin[2].setup(DIGIN3_PIN, INPUT);
  #endif
  #if DIGIN_COUNT > 3
    digInPin[3].setup(DIGIN4_PIN, INPUT);
  #endif
  #if DIGIN_COUNT > 4
    digInPin[4].setup(DIGIN5_PIN, INPUT);
  #endif
  #if DIGIN_COUNT > 5
    digInPin[5].setup(DIGIN6_PIN, INPUT);
  #endif
  #endif
}

void resetOutputs() {
  for (byte zone = 0; zone < NUM_ZONES; zone++) setpoint[zone] = NO_SETPOINT;
  actHeats = actCools = 0;
  updateValves();
}

void processOutputs() {
  for (byte zone = 0; zone < NUM_ZONES; zone++) {
    if (setpoint[zone] == NO_SETPOINT) {
      zonePwr[zone] = 0;
      eventHandler(EVENT_NALARM_TEMPHOT, zone); //Clear TEMPHOT Alarm
      eventHandler(EVENT_NALARM_TEMPCOLD, zone); //Clear TEMPCOLD Alarm
      eventHandler(EVENT_NALARM_TSENSOR, zone); //Clear TSENSOR Alarm
    }
    else if (temp[zone] == BAD_TEMP && !bitSet(alarmStatus[zone], ALARM_STATUS_TSENSOR)) {
      eventHandler(EVENT_ALARM_TSENSOR, zone);
      zonePwr[zone] = 0;
    }
    else
    {
      // Check for alarms
      if (bitRead(alarmStatus[zone], ALARM_STATUS_TSENSOR)) eventHandler(EVENT_NALARM_TSENSOR, zone); //Clear TSENSOR Alarm
      
      if (temp[zone] - setpoint[zone] >= alarmThresh[zone] * 10)
      {
       if (!bitRead(alarmStatus[zone], ALARM_STATUS_TEMPHOT)) eventHandler(EVENT_ALARM_TEMPHOT, zone);
      }
      else if (bitRead(alarmStatus[zone], ALARM_STATUS_TEMPHOT)) eventHandler(EVENT_NALARM_TEMPHOT, zone); //Clear TEMPHOT Alarm
  
      if (setpoint[zone] - temp[zone] >= alarmThresh[zone] * 10)
      {
       if (!bitRead(alarmStatus[zone], ALARM_STATUS_TEMPCOLD)) eventHandler(EVENT_ALARM_TEMPCOLD, zone);
      }
      else if (bitRead(alarmStatus[zone], ALARM_STATUS_TEMPCOLD)) eventHandler(EVENT_NALARM_TEMPCOLD, zone); //Clear TEMPCOLD Alarm
      
      // check for output on & off
      if (zonePwr[zone] > 0 && temp[zone] >= setpoint[zone]) zonePwr[zone] = 0; //Turn off heat

      // If we're cooling, and it's over the max time, we need to stop
      else if(zonePwr[zone] < 0)
      {
       unsigned long now = millis();
       if (now < coolTime[zone]) coolTime[zone] = 0; //Timer overflow occurred
       if (now - coolTime[zone] >= (unsigned long) coolMaxOn[zone] * 60000) {
        zonePwr[zone] = 0; //Turn off cool
        coolTime[zone] = now; //Set timer for minimum off period
       }
      }

      //If we're cooling, are we done, with temp and minimum time?
      else if(zonePwr[zone] < 0 && temp[zone] <= setpoint[zone])
      {
        //Check for minimum cool on period
        unsigned long now = millis();
        if (now < coolTime[zone]) coolTime[zone] = 0; //Timer overflow occurred
        if (now - coolTime[zone] >= (unsigned long) coolMinOn[zone] * 60000) {
          zonePwr[zone] = 0; //Turn off cool
          coolTime[zone] = now; //Set timer for minimum off period
        }
      }
      
      // The temp is too warm, should we turn cool on?
      if (temp[zone] >= setpoint[zone] + (int)hysteresis[zone] * 10)
      {
        //Check for minimum cool off period
        unsigned long now = millis();
        if (now < coolTime[zone]) coolTime[zone] = 0; //Timer overflow occurred
        if (now - coolTime[zone] >= (unsigned long) coolMinOff[zone] * 60000) {
          zonePwr[zone] = -100; //Cool On
          coolTime[zone] = now; //Set timer for minimum on period
        }
      }

      if (temp[zone] <= setpoint[zone] - (int)hysteresis[zone] * 10) zonePwr[zone] = 100;  //Heat On
    }
    
    if (zonePwr[zone] < 1) bitClear(actHeats, zone); else bitSet(actHeats, zone);
    if (zonePwr[zone] > -1) bitClear(actCools, zone); else bitSet(actCools, zone);
  }
}

unsigned long prevHeats, prevCools;
boolean prevBuzz;

void updateValves() {
  if (actHeats != prevHeats || actCools != prevCools || buzzStatus != prevBuzz) {
    Valves.set(computeValveBits());
    prevHeats = actHeats;
    prevCools = actCools;
    prevBuzz = buzzStatus;
  }
}

unsigned long computeValveBits() {
  unsigned long vlvBits = 0;
  for (byte i = 0; i < NUM_ZONES; i++) {
    if (bitRead(actHeats, i)) vlvBits |= vlvConfig[i];
    if (bitRead(actCools, i)) vlvBits |= vlvConfig[NUM_ZONES + i];
  }
  if (buzzStatus) vlvBits |= vlvConfig[VLV_ALARM];
  return vlvBits;
}

boolean vlvConfigIsActive(byte profile) {
  //An empty valve profile cannot be active
  if (!vlvConfig[profile]) return 0;
  if (profile < NUM_ZONES) return bitRead(actHeats, profile);
  else return bitRead(actCools, profile);
}

boolean isAlarmAllZones() {
  for (byte zone = 0; zone < NUM_ZONES; zone++) if (alarmStatus[zone]) return 1;
  return 0;
}

void updateAlarm() {
  for (byte zone = 0; zone < NUM_ZONES; zone++) {
     {
      if (alarmStatus[zone] & ALARM_ACKBITS) {
        setBuzzer(1); //ACK Required bit(s) set; sound alarm
        return;
      }
    }
  }
  setBuzzer(0); //Made it this far; clear alarm
}

//This function allow to modulate the sound of the buzzer when the alarm is ON. 
//The modulation varies according the custom parameters.
//The modulation occurs when the buzzerCycleTime value is larger than the buzzerOnDuration
void setBuzzer(boolean alarmON) {
  if (alarmON) {
    #ifdef BUZZER_CYCLE_TIME
      //Alarm status is ON, Buzzer will go ON or OFF based on modulation.
      //The buzzer go OFF for every moment passed in the OFF window (low duty cycle). 
      unsigned long now = millis(); //What time is it? :-))      
      
      if (now() < buzzerCycleStart) buzzerCycleStart = 0; //Timer overflow occurred
      
      //Now, by elimation, identify scenarios where the buzzer will go off. 
      if (now < buzzerCycleStart + BUZZER_CYCLE_TIME) {
        //At this moment ("now"), the buzzer is in the OFF window (low duty cycle). 
        if (now > buzzerCycleStart + BUZZER_ON_TIME) {
          //At this moment ("now"), the buzzer is NOT within the ON window (duty cycle) allowed inside the buzzer cycle window.
          //Set or keep the buzzer off
          buzzStatus = 0;
        }
      } else {
        //The buzzer go ON for every moment where buzzerCycleStart < "now" < buzzerCycleStart + buzzerOnDuration
        buzzStatus = 1; //Set the buzzer On 
        buzzerCycleStart = now; //Set a new reference time for the begining of the buzzer cycle.
      }
    #else
      buzzStatus = 1; //Set the buzzer On 
    #endif
  } else {
    //Alarm status is OFF, Buzzer goes Off
    buzzStatus = 0;
  }
}
