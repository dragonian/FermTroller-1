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

#include "Config.h"
#include "Enum.h"
#include <avr/eeprom.h>
#include <EEPROM.h>
#include "FermTroller.h"  
#include "EEPROM.h"
#include "Outputs.h"

void loadSetup() {
  //**********************************************************************************
  // TSensors: 8 bytes per zone (Reserve for 32 zones: 0-255)
  //**********************************************************************************
  EEPROMreadBytes(0, *tSensor, NUM_ZONES * 8);
 
 
  //**********************************************************************************
  // Per Zone Settings
  //**********************************************************************************
  for (byte zone = 0; zone < NUM_ZONES; zone++) {

    //********************************************************************************
    // Hysteresis (256 - 287)
    //********************************************************************************
    hysteresis[zone] = EEPROM.read(256 + zone);

    //********************************************************************************
    // Alarm Threshhold (288 - 319)
    //********************************************************************************
    alarmThresh[zone] = EEPROM.read(288 + zone);

    //********************************************************************************
    // Alarm Status (320 - 351)
    //********************************************************************************
    alarmStatus[zone] = EEPROM.read(320 + zone);

    //********************************************************************************
    // Minimum Cool Output Active Period (352 - 383)
    //********************************************************************************
    coolMinOn[zone] = EEPROM.read(352 + zone);
    
    //********************************************************************************
    // Minimum Cool Output Inactive Period (384 - 415)
    //********************************************************************************
    coolMinOff[zone] = EEPROM.read(384 + zone);

    //********************************************************************************
    // Max Cool Output Active Period (416 - 447)
    //********************************************************************************
    coolMaxOn[zone] = EEPROM.read(416 + zone);

    //********************************************************************************
    // Setpoints: 2 Bytes per zone (640-703)
    //********************************************************************************
    setpoint[zone] = EEPROMreadInt(640 + zone * 2);
  }
  
  //**********************************************************************************
  // Valve Profiles (705-964): 4 Bytes per profile, 2 profiles per zone plus alarm profile
  //**********************************************************************************
  loadVlvConfigs();
}

void loadVlvConfigs() {
  eeprom_read_block(&vlvConfig, (unsigned char *) 705, 4 * (NUM_ZONES * 2 + 1));
}

//*****************************************************************************************************************************
// Individual EEPROM Get/Set Variable Functions
//*****************************************************************************************************************************

//**********************************************************************************
// TSensors: 8 bytes per zone (Reserve for 32 zones: 0-255)
//**********************************************************************************
void setTSAddr(byte zone, byte addr[8]) {
  memcpy(tSensor[zone], addr, 8);
  EEPROMwriteBytes(zone * 8, addr, 8);
}


//**********************************************************************************
// Hysteresis (256 - 287)
//**********************************************************************************
void setHysteresis(byte output, byte value) {
  hysteresis[output] = value;
  EEPROM.write(256 + output, value);
}

//**********************************************************************************
// Alarm Threshhold (288 - 319)
//**********************************************************************************
void setAlarmThresh(byte output, byte value) {
  alarmThresh[output] = value;
  EEPROM.write(288 + output, value);
}

//**********************************************************************************
// Alarm Status (320 - 351)
//**********************************************************************************
void setAlarmStatus(byte zone, byte value) {
  alarmStatus[zone] = value;
  saveAlarmStatus(zone);
}

void saveAlarmStatus(byte zone) {
  EEPROM.write(320 + zone, alarmStatus[zone]);
}

//********************************************************************************
// Minimum Cool Output Active Period (352 - 383)
//********************************************************************************
void setCoolMinOn(byte zone, byte value) {
  coolMinOn[zone] = value;
  EEPROM.write(352 + zone, value);
}

//********************************************************************************
// Minimum Cool Output Inactive Period (384 - 415)
//********************************************************************************
void setCoolMinOff(byte zone, byte value) {
  coolMinOff[zone] = value;
  EEPROM.write(384 + zone, value);
}

//********************************************************************************
// Max Cool Output Active Period (416 - 447)
//********************************************************************************
void setCoolMaxOn(byte zone, byte value) {
  coolMaxOn[zone] = value;
  EEPROM.write(416 + zone, value);
}

//*****************************************************************************************************************************
// Power Loss Recovery Functions
//*****************************************************************************************************************************

//**********************************************************************************
// Setpoints: 2 Bytes per zone (640-703)
//**********************************************************************************
void setSetpoint(byte zone, int value) {
  setpoint[zone] = value;
  EEPROMwriteInt(640 + zone * 2, value);
}

//*****************************************************************************************************************************
// Valve Profiles (705-964) 4 Bytes per profile, 2 profiles per zone plus alarm profile
//*****************************************************************************************************************************
void setValveCfg(byte profile, unsigned long value) {
  vlvConfig[profile] = value;
  EEPROMwriteLong(705 + profile * 4, value);
}

//*****************************************************************************************************************************
// Zone names (965 - 1540) 18 bytes per zone (17 chars + NULL)
//*****************************************************************************************************************************
char* getZoneName(byte zone, char name[]) {
  memset(name, ' ', 17);
  name[17] = '\0';
  for (byte chr = 0; chr < 18; chr++) name[chr] = EEPROM.read(965 + zone * 18 + chr);
  return name;
}

void setZoneName(byte zone, char name[]) {
  for (byte chr = 0; chr < 18; chr++) EEPROM.write(965 + zone * 18 + chr, name[chr]);
}




//**********************************************************************************
//FermTroller Fingerprint (2046)
//**********************************************************************************

//**********************************************************************************
//EEPROM Version (2047)
//**********************************************************************************

//**********************************************************************************
//LCD Bright/Contrast (2048-2049) ATMEGA1284P Only
//**********************************************************************************


//*****************************************************************************************************************************
// Check/Update/Format EEPROM
//*****************************************************************************************************************************
boolean checkConfig() {
  byte cfgVersion = EEPROM.read(2047);
  byte BTFinger = EEPROM.read(2046);

  //If the fingerprint is missing force a init of EEPROM
  if (BTFinger != 253 || cfgVersion == 255 || cfgVersion < 7) {
    #if (defined __AVR_ATmega1284P__ || defined __AVR_ATmega1284__) && defined UI_DISPLAY_SETUP && defined UI_LCD_4BIT
      EEPROM.write(2048, LCD_DEFAULT_BRIGHTNESS);
      EEPROM.write(2049, LCD_DEFAULT_CONTRAST);
    #endif
    return 1;
  }
  
  //In the future, incremental EEPROM settings will be included here
  switch(cfgVersion) {

  }
  return 0;
}

void initEEPROM() {
  //Format EEPROM to 0's
  for (int i=0; i<2048; i++) EEPROM.write(i, 0);

  //Set FT Fingerprint (253)
  EEPROM.write(2046, 253);

  //Default Output Settings: p: 3, i: 4, d: 2, cycle: 4s, Hysteresis 0.3C(0.5F)
  for (byte zone = 0; zone <= NUM_ZONES; zone++) {
    #ifdef USEMETRIC
      setHysteresis(zone, 3);
      setAlarmThresh(zone, 6);
    #else
      setHysteresis(zone, 5);
      setAlarmThresh(zone, 10);
    #endif
    
    setSetpoint(zone, NO_SETPOINT);

    char name[19];
    strcpy_P(name, PSTR("Zone "));
    itoa(zone + 1, buf, 10);
    strcat(name, buf);
    byte len = strlen(name);
    for (byte i = len; i < 17; i++) name[i] = ' ';
    name[17] = '\0';
    setZoneName(zone, name);
  }
  
  //Set default LCD Bright/Contrast
  #if (defined __AVR_ATmega1284P__ || defined __AVR_ATmega1284__) && defined UI_DISPLAY_SETUP && defined UI_LCD_4BIT
    EEPROM.write(2048, LCD_DEFAULT_BRIGHTNESS);
    EEPROM.write(2049, LCD_DEFAULT_CONTRAST);
  #endif
  
  //Set cfgVersion = 7
  EEPROM.write(2047, 7);

  // re-load Setup 
  loadSetup();
  LCD.init();
}

//*****************************************************************************************************************************
// EEPROM Type Read/Write Functions
//*****************************************************************************************************************************
long EEPROMreadLong(int address) {
  long out;
  eeprom_read_block((void *) &out, (unsigned char *) address, 4);
  return out;
}

void EEPROMwriteLong(int address, long value) {
  eeprom_write_block((void *) &value, (unsigned char *) address, 4);
}

int EEPROMreadInt(int address) {
  int out;
  eeprom_read_block((void *) &out, (unsigned char *) address, 2);
  return out;
}

void EEPROMwriteInt(int address, int value) {
  eeprom_write_block((void *) &value, (unsigned char *) address, 2);
}

void EEPROMwriteBytes(int addr, byte bytes[], byte numBytes) {
  for (byte i = 0; i < numBytes; i++) {
    EEPROM.write(addr + i, bytes[i]);
  }
}

void EEPROMreadBytes(int addr, byte bytes[], byte numBytes) {
  for (byte i = 0; i < numBytes; i++) {
    bytes[i] = EEPROM.read(addr + i);
  }
}

