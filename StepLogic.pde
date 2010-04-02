/*  
   Copyright (C) 2009, 2010 Matt Reba, Jermeiah Dillingham

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

Compiled on Arduino-0017 (http://arduino.cc/en/Main/Software)
With Sanguino Software v1.4 (http://code.google.com/p/sanguino/downloads/list)
using PID Library v0.6 (Beta 6) (http://www.arduino.cc/playground/Code/PIDLibrary)
using OneWire Library (http://www.arduino.cc/playground/Learning/OneWire)
*/

unsigned long lastHop;
unsigned int boilAdds, triggered;

boolean stepIsActive(byte brewStep) {
  if (stepProgram[brewStep] != PROGRAM_IDLE) return true; else return false;
}

boolean zoneIsActive(byte brewZone) {
  if (brewZone == ZONE_MASH) {
    if (stepIsActive(STEP_FILL) 
      || stepIsActive(STEP_DELAY) 
      || stepIsActive(STEP_PREHEAT)
      || stepIsActive(STEP_ADDGRAIN) 
      || stepIsActive(STEP_REFILL)
      || stepIsActive(STEP_DOUGHIN) 
      || stepIsActive(STEP_ACID)
      || stepIsActive(STEP_PROTEIN) 
      || stepIsActive(STEP_SACCH)
      || stepIsActive(STEP_SACCH2) 
      || stepIsActive(STEP_MASHOUT)
      || stepIsActive(STEP_MASHHOLD) 
      || stepIsActive(STEP_SPARGE)
    ) return 1; else return 0;
  } else if (brewZone == ZONE_BOIL) {
    if (stepIsActive(STEP_BOIL) 
      || stepIsActive(STEP_CHILL) 
    ) return 1; else return 0;
  }
}

void stepCore() {
  if (stepIsActive(STEP_FILL)) stepFill(STEP_FILL);
  if (stepIsActive(STEP_PREHEAT)) stepMash(STEP_PREHEAT);
  if (stepIsActive(STEP_DELAY)) if (timerValue[TIMER_MASH] == 0) stepAdvance(STEP_DELAY);
  if (stepIsActive(STEP_ADDGRAIN)) { /*Nothing much happens*/ }
  if (stepIsActive(STEP_REFILL)) stepFill(STEP_REFILL);
  for (byte brewStep = STEP_DOUGHIN; brewStep <= STEP_MASHOUT; brewStep++) if (stepIsActive(brewStep)) stepMash(brewStep);
  
  if (stepIsActive(STEP_MASHHOLD)) {
    #ifdef SMART_HERMS_HLT
      smartHERMSHLT();
    #endif
    #ifdef AUTO_MASH_HOLD_EXIT
      if (!zoneIsActive(ZONE_BOIL)) stepAdvance(STEP_MASHHOLD);
    #endif
  }
  
  if (stepIsActive(STEP_SPARGE)) { 

  }
  
  if (stepIsActive(STEP_BOIL)) {
    if (doAutoBoil) {
      if(temp[TS_KETTLE] < setpoint[TS_KETTLE]) PIDOutput[VS_KETTLE] = PIDCycle[VS_KETTLE] * 10 * PIDLIMIT_KETTLE;
      else PIDOutput[VS_KETTLE] = PIDCycle[VS_KETTLE] * 10 * min(boilPwr, PIDLIMIT_KETTLE);
    }
    #ifdef PREBOIL_ALARM
      if ((triggered ^ 32768) && temp[TS_KETTLE] >= PREBOIL_ALARM) {
        setAlarm(1);
        triggered |= 32768; 
        setABAddsTrig(triggered);
      }
    #endif
    if (!preheated[VS_KETTLE] && temp[TS_KETTLE] >= setpoint[TS_KETTLE] && setpoint[TS_KETTLE] > 0) {
      preheated[VS_KETTLE] = 1;
      //Unpause Timer
      if (!timerStatus[TIMER_BOIL]) pauseTimer(TIMER_BOIL);
    }
    //Turn off hop valve profile after 5s
    if ((vlvConfigIsActive(VLV_HOPADD)) && lastHop > 0 && millis() - lastHop > HOPADD_DELAY) {
      setValves(vlvConfig[VLV_HOPADD], 0);
      lastHop = 0;
    }
    if (preheated[VS_KETTLE]) {
      //Boil Addition
      if ((boilAdds ^ triggered) & 1) {
        setValves(vlvConfig[VLV_HOPADD], 1);
        lastHop = millis();
        setAlarm(1); 
        triggered |= 1; 
        setBoilAddsTrig(triggered); 
      }
      //Timed additions (See hoptimes[] array at top of AutoBrew.pde)
      for (byte i = 0; i < 10; i++) {
        if (((boilAdds ^ triggered) & (1<<(i + 1))) && timerValue[TIMER_BOIL] <= hoptimes[i] * 60000) { 
          setValves(vlvConfig[VLV_HOPADD], 1);
          lastHop = millis();
          setAlarm(1); 
          triggered |= (1<<(i + 1)); 
          setBoilAddsTrig(triggered);
        }
      }
      #ifdef AUTO_BOIL_RECIRC
      if (timerValue[TIMER_BOIL] <= AUTO_BOIL_RECIRC * 60000) setValves(vlvConfig[VLV_BOILRECIRC], 1);
      #endif
    }
    //Exit Condition  
    if(preheated[VS_KETTLE] && timerValue == 0) stepAdvance(STEP_BOIL);
  }
  
  if (stepIsActive(STEP_CHILL)) {
    if (temp[TS_KETTLE] != -1 && temp[TS_KETTLE] <= KETTLELID_THRESH) {
      if (!vlvConfigIsActive(VLV_KETTLELID)) setValves(vlvConfig[VLV_KETTLELID], 1);
    } else {
      if (vlvConfigIsActive(VLV_KETTLELID)) setValves(vlvConfig[VLV_KETTLELID], 0);
    }
  }
}

//stepCore logic for Fill and Refill
void stepFill(byte brewStep) {
  #ifdef AUTO_FILL
    if (volAvg[VS_HLT] >= tgtVol[VS_HLT] && volAvg[VS_MASH] >= tgtVol[VS_MASH]) stepAdvance(brewStep);
  #endif
}

//stepCore Logic for Preheat and all mash steps
void stepMash(byte brewStep) {
  #ifdef SMART_HERMS_HLT
    smartHERMSHLT();
  #endif
  if (!preheated[VS_MASH] && temp[TS_MASH] >= setpoint[VS_MASH]) {
    preheated[VS_MASH] = 1;
    //Unpause Timer
    if (!timerStatus[TIMER_MASH]) pauseTimer(TIMER_MASH);
  }
  if (preheated[VS_MASH] && timerValue == 0) stepAdvance(brewStep);
}

//Returns 0 if start was successful or 1 if unable to start due to conflict with other step
//Performs any logic required at start of step
//TO DO: Power Loss Recovery Handling
boolean stepInit(byte pgm, byte brewStep) {
  
  //Abort Fill/Mash step init if mash Zone is not free
  if (brewStep >= STEP_FILL && brewStep <= STEP_MASHHOLD && zoneIsActive(ZONE_MASH)) return 1;  
  //Abort sparge init if either zone is currently active
  else if (brewStep == STEP_SPARGE && (zoneIsActive(ZONE_MASH) || zoneIsActive(ZONE_BOIL))) return 1;  
  //Allow Boil step init while sparge is still going

  //If we made it without an abort, save the program number for stepCore
  setProgramStep(brewStep, pgm);

  if (brewStep == STEP_FILL) {
  //Step Init: Fill
    //Set Target Volumes
    tgtVol[TS_HLT] = calcSpargeVol(pgm);
    tgtVol[TS_MASH] = calcMashVol(pgm);
    if (getProgMLHeatSrc(pgm) == VS_HLT) {
      tgtVol[VS_HLT] = min(tgtVol[VS_HLT] + tgtVol[VS_MASH], getCapacity(VS_HLT));
      tgtVol[VS_MASH] = 0;
    }
    #ifdef AUTO_FILL
      autoValve[AV_FILL] = 1;
    #endif

  } else if (brewStep == STEP_DELAY) {
  //Step Init: Delay
    //Load delay minutes from EEPROM if timer is not already populated via Power Loss Recovery
    if (!timerValue[TIMER_MASH]) setTimer(TIMER_MASH, getDelayMins());

  } else if (brewStep == STEP_PREHEAT) {
  //Step Init: Preheat
    //Find first temp and adjust for strike temp
    {
      if (getProgMLHeatSrc(pgm) == VS_HLT) {
        setpoint[TS_HLT] = calcStrikeTemp(pgm);
        #ifdef STRIKE_TEMP_OFFSET
          setpoint[TS_HLT] += STRIKE_TEMP_OFFSET;
        #endif
        setpoint[TS_MASH] = 0;
      } else {
        setpoint[TS_HLT] = getProgHLT(pgm);
        setpoint[TS_MASH] = calcStrikeTemp(pgm);
      }
      setpoint[VS_STEAM] = getSteamTgt();
      pid[VS_HLT].SetMode(AUTO);
      pid[VS_MASH].SetMode(AUTO);
      pid[VS_STEAM].SetMode(AUTO);
    }
    preheated[VS_MASH] = 0;
    autoValve[AV_MASH] = 1;
    //No timer used for preheat
    clearTimer(TIMER_MASH);
    
  } else if (brewStep == STEP_ADDGRAIN) {
  //Step Init: Add Grain
    //Disable HLT and Mash heat output during 'Add Grain' to avoid dry running heat elements and burns from HERMS recirc
    resetHeatOutput(VS_HLT);
    resetHeatOutput(VS_MASH);
    setpoint[VS_STEAM] = getSteamTgt();
    setValves(vlvConfig[VLV_ADDGRAIN], 1);

  } else if (brewStep == STEP_REFILL) {
  //Step Init: Refill
    if (getProgMLHeatSrc(pgm) == VS_HLT) {
      tgtVol[VS_HLT] = calcSpargeVol(pgm);
      tgtVol[VS_MASH] = 0;
    }

  } else if (brewStep == STEP_DOUGHIN) {
  //Step Init: Dough In
    setpoint[TS_HLT] = getProgHLT(pgm);
    setpoint[TS_MASH] = getProgMashTemp(pgm, MASH_DOUGHIN);
    setpoint[VS_STEAM] = getSteamTgt();
    pid[VS_HLT].SetMode(AUTO);
    pid[VS_MASH].SetMode(AUTO);
    pid[VS_STEAM].SetMode(AUTO);
    preheated[VS_MASH] = 0;
    autoValve[AV_MASH] = 1;
    //Set timer only if empty (for purposed of power loss recovery)
    if (!timerValue[TIMER_MASH]) setTimer(TIMER_MASH, getProgMashMins(pgm, MASH_DOUGHIN)); 
    //Leave timer paused until preheated
    timerStatus[TIMER_MASH] = 0;
    
  } else if (brewStep == STEP_ACID) {
  //Step Init: Acid Rest
    setpoint[TS_HLT] = getProgHLT(pgm);
    setpoint[TS_MASH] = getProgMashTemp(pgm, MASH_ACID);
    setpoint[VS_STEAM] = getSteamTgt();
    pid[VS_HLT].SetMode(AUTO);
    pid[VS_MASH].SetMode(AUTO);
    preheated[VS_MASH] = 0;
    autoValve[AV_MASH] = 1;
    //Set timer only if empty (for purposed of power loss recovery)
    if (!timerValue[TIMER_MASH]) setTimer(TIMER_MASH, getProgMashMins(pgm, MASH_ACID)); 
    //Leave timer paused until preheated
    timerStatus[TIMER_MASH] = 0;
    
  } else if (brewStep == STEP_PROTEIN) {
  //Step Init: Protein
    setpoint[TS_HLT] = getProgHLT(pgm);
    setpoint[TS_MASH] = getProgMashTemp(pgm, MASH_PROTEIN);
    setpoint[VS_STEAM] = getSteamTgt();
    pid[VS_HLT].SetMode(AUTO);
    pid[VS_MASH].SetMode(AUTO);
    preheated[VS_MASH] = 0;
    autoValve[AV_MASH] = 1;
    //Set timer only if empty (for purposed of power loss recovery)
    if (!timerValue[TIMER_MASH]) setTimer(TIMER_MASH, getProgMashMins(pgm, MASH_PROTEIN)); 
    //Leave timer paused until preheated
    timerStatus[TIMER_MASH] = 0;
    
  } else if (brewStep == STEP_SACCH) {
  //Step Init: Sacch
    setpoint[TS_HLT] = getProgHLT(pgm);
    setpoint[TS_MASH] = getProgMashTemp(pgm, MASH_SACCH);
    setpoint[VS_STEAM] = getSteamTgt();
    pid[VS_HLT].SetMode(AUTO);
    pid[VS_MASH].SetMode(AUTO);
    pid[VS_STEAM].SetMode(AUTO);
    preheated[VS_MASH] = 0;
    autoValve[AV_MASH] = 1;
    //Set timer only if empty (for purposed of power loss recovery)
    if (!timerValue[TIMER_MASH]) setTimer(TIMER_MASH, getProgMashMins(pgm, MASH_SACCH)); 
    //Leave timer paused until preheated
    timerStatus[TIMER_MASH] = 0;
    
  } else if (brewStep == STEP_SACCH2) {
  //Step Init: Sacch2
    setpoint[TS_HLT] = getProgHLT(pgm);
    setpoint[TS_MASH] = getProgMashTemp(pgm, MASH_SACCH2);
    setpoint[VS_STEAM] = getSteamTgt();
    pid[VS_HLT].SetMode(AUTO);
    pid[VS_MASH].SetMode(AUTO);
    preheated[VS_MASH] = 0;
    autoValve[AV_MASH] = 1;
    //Set timer only if empty (for purposed of power loss recovery)
    if (!timerValue[TIMER_MASH]) setTimer(TIMER_MASH, getProgMashMins(pgm, MASH_SACCH2)); 
    //Leave timer paused until preheated
    timerStatus[TIMER_MASH] = 0;
    
  } else if (brewStep == STEP_MASHOUT) {
  //Step Init: Mash Out
    setpoint[TS_HLT] = getProgHLT(pgm);
    setpoint[TS_MASH] = getProgMashTemp(pgm, MASH_MASHOUT);
    setpoint[VS_STEAM] = getSteamTgt();
    pid[VS_HLT].SetMode(AUTO);
    pid[VS_MASH].SetMode(AUTO);
    pid[VS_STEAM].SetMode(AUTO);
    preheated[VS_MASH] = 0;
    autoValve[AV_MASH] = 1;
    //Set timer only if empty (for purposed of power loss recovery)
    if (!timerValue[TIMER_MASH]) setTimer(TIMER_MASH, getProgMashMins(pgm, MASH_MASHOUT)); 
    //Leave timer paused until preheated
    timerStatus[TIMER_MASH] = 0;
    
  } else if (brewStep == STEP_MASHHOLD) {
    //Set HLT to Sparge Temp
    setpoint[TS_HLT] = getProgSparge(pgm);
    //Cycle through steps and use last non-zero step for mash setpoint
    if (!setpoint[TS_MASH]) {
      byte i = MASH_MASHOUT;
      while (setpoint[TS_MASH] == 0 && i >= MASH_DOUGHIN && i <= MASH_MASHOUT) setpoint[TS_MASH] = getProgMashTemp(pgm, i--);
    }
    setpoint[VS_STEAM] = getSteamTgt();
    pid[VS_HLT].SetMode(AUTO);
    pid[VS_MASH].SetMode(AUTO);
    pid[VS_STEAM].SetMode(AUTO);

  } else if (brewStep == STEP_SPARGE) {
  //Step Init: Sparge


  } else if (brewStep == STEP_BOIL) {
  //Step Init: Boil
    setpoint[TS_KETTLE] = getBoilTemp();
    preheated[VS_KETTLE] = 0;
    triggered = getBoilAddsTrig();
    //Set timer only if empty (for purposed of power loss recovery)
    if (!timerValue[TIMER_BOIL]) setTimer(TIMER_BOIL, getProgBoil(pgm));
    //Leave timer paused until preheated
    timerStatus[TIMER_MASH] = 0;
    lastHop = 0;
    doAutoBoil = 1;
    
  } else if (brewStep == STEP_CHILL) {
  //Step Init: Chill
  }
  
  return 0;
}

//Advances program to next brew step
//Returns 0 if successful or 1 if unable to advance due to conflict with another step
boolean stepAdvance(byte brewStep) {
  //Save program for next step/rollback
  byte program = stepProgram[brewStep];
  stepExit(brewStep);
  //Advance step (if applicable)
  if (brewStep + 1 < NUM_BREW_STEPS) {
    if (stepInit(brewStep + 1, program)) {
      //Init Failed: Rollback
      stepExit(brewStep + 1); //Just to make sure we clean up a partial start
      setProgramStep(brewStep, program); //Show the step we started with as active
      return 1;
    }
    //Init Successful
    return 0;
  }
}

//Performs exit logic specific to each step
//Note: If called directly (as opposed through stepAdvance) acts as a program abort
void stepExit(byte brewStep) {
  //Mark step idle
  setProgramStep(brewStep, PROGRAM_IDLE);
  
  //Perform step closeout functions
  if (brewStep == STEP_FILL || brewStep == STEP_REFILL) {
  //Step Exit: Fill/Refill
    tgtVol[VS_HLT] = 0;
    tgtVol[VS_MASH] = 0;
    autoValve[AV_FILL] = 0;
    setValves(vlvConfig[VLV_FILLHLT], 0);
    setValves(vlvConfig[VLV_FILLMASH], 0);

  } else if (brewStep == STEP_DELAY) {
  //Step Exit: Delay
    clearTimer(TIMER_MASH);
  
  } else if (brewStep == STEP_ADDGRAIN) {
  //Step Exit: Add Grain
    setValves(vlvConfig[VLV_ADDGRAIN], 0);    
    resetHeatOutput(VS_HLT);
#ifdef USESTEAM
    resetHeatOutput(VS_STEAM);
#endif

  } else if (brewStep == STEP_PREHEAT || (brewStep >= STEP_DOUGHIN && brewStep <= STEP_MASHHOLD)) {
  //Step Exit: Preheat/Mash
    clearTimer(TIMER_MASH);
    autoValve[AV_MASH] = 0;
    setValves(vlvConfig[VLV_MASHHEAT], 0);    
    setValves(vlvConfig[VLV_MASHIDLE], 0);   
    resetHeatOutput(VS_HLT);
    resetHeatOutput(VS_MASH);
#ifdef USESTEAM
    resetHeatOutput(VS_STEAM);
#endif

  } else if (brewStep == STEP_SPARGE) {
  //Step Exit: Sparge
    autoValve[AV_SPARGE] = 0;
    setValves(vlvConfig[VLV_SPARGEIN], 0);    
    setValves(vlvConfig[VLV_SPARGEOUT], 0);    

  } else if (brewStep == STEP_BOIL) {
  //Step Exit: Boil
    //0 Min Addition
    if ((boilAdds ^ triggered) & 2048) { 
      setValves(vlvConfig[VLV_HOPADD], 1);
      setAlarm(1);
      triggered |= 2048;
      setBoilAddsTrig(triggered);
      delay(HOPADD_DELAY);
    }
    setValves(vlvConfig[VLV_HOPADD], 0);
    #ifdef AUTO_BOIL_RECIRC
      setValves(vlvConfig[VLV_BOILRECIRC], 0);
    #endif
    resetHeatOutput(VS_KETTLE);
    
  } else if (brewStep == STEP_CHILL) {
  //Step Exit: Chill
    autoValve[AV_CHILL] = 0;
    setValves(vlvConfig[VLV_CHILLBEER], 0);    
    setValves(vlvConfig[VLV_CHILLH2O], 0);  
  }
}

#ifdef SMART_HERMS_HLT
void smartHERMSHLT() {
  if (setpoint[VS_MASH] != 0) setpoint[VS_HLT] = constrain(setpoint[VS_MASH] * 2 - temp[TS_MASH], setpoint[VS_MASH] + MASH_HEAT_LOSS, HLT_MAX_TEMP);
}
#endif
  
unsigned long calcMashVol(byte pgm) {
  unsigned long retValue = round(getProgGrain(pgm) * getProgRatio(pgm) / 100.0);
  //Convert qts to gal for US
  #ifndef USEMETRIC
    retValue = round(retValue / 4.0);
  #endif
  return retValue;
}

unsigned long calcSpargeVol(byte pgm) {
  //Detrmine Total Water Needed (Evap + Deadspaces)
  unsigned long retValue = round(getProgBatchVol(pgm) / (1.0 - getEvapRate() / 100.0 * getProgBoil(pgm) / 60.0) + getVolLoss(TS_HLT) + getVolLoss(TS_MASH));
  //Add Water Lost in Spent Grain
  #ifdef USEMETRIC
    retValue += round(getProgGrain(pgm) * 1.7884);
  #else
    retValue += round(getProgGrain(pgm) * .2143);
  #endif

  //Subtract mash volume
  retValue -= calcMashVol(pgm);
  return retValue;
}

unsigned long calcGrainVolume(byte pgm) {
  //Grain-to-volume factor for mash tun capacity
  //Conservatively 1 lb = 0.15 gal 
  //Aggressively 1 lb = 0.093 gal
  #ifdef USEMETRIC
    #define GRAIN2VOL 1.25
  #else
    #define GRAIN2VOL 0.15
  #endif
  return round (getProgGrain(pgm) * GRAIN2VOL);
}

byte calcStrikeTemp(byte pgm) {
  byte strikeTemp = 0;
  byte i = MASH_DOUGHIN;
  while (strikeTemp == 0 && i <= MASH_MASHOUT) strikeTemp = getProgMashTemp(pgm, i++);
  #ifdef USEMETRIC
    return strikeTemp + round(.4 * (strikeTemp - getGrainTemp()) / (getProgRatio(pgm) / 100.0)) + 1.7;
  #else
    return strikeTemp + round(.192 * (strikeTemp - getGrainTemp()) / (getProgRatio(pgm) / 100.0)) + 3;
  #endif
}
