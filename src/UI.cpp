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

#ifndef NOUI
#include "Config.h"
#include "Enum.h"
#include "HWProfile.h"
#include <encoder.h>
#include "UI_LCD.h"

#include "FermTroller.h"
#include "Outputs.h"
#include "EEPROM.h"
#include "UI.h"
#include "Util.h"
#include "FermCore.h"

//*****************************************************************************************************************************
// Begin UI Code
//*****************************************************************************************************************************


//**********************************************************************************
// UI Definitions
//**********************************************************************************
#define SCREEN_HOME 0
#define SCREEN_FILL 1
#define SCREEN_MASH 2
#define SCREEN_SPARGE 3
#define SCREEN_BOIL 4
#define SCREEN_CHILL 5
#define SCREEN_AUX 6

//**********************************************************************************
// UI Strings
//**********************************************************************************
const char OK[] PROGMEM = "Ok";
const char CANCEL[] PROGMEM = "Cancel";
const char EXIT[] PROGMEM = "Exit";
const char ABORT[] PROGMEM = "Abort";
const char MENU[] PROGMEM = "Menu";
const char SPACE[] PROGMEM = " ";
const char INIT_EEPROM[] PROGMEM = "Initialize EEPROM";
const char CONTINUE[] PROGMEM = "Continue";
const char ZONE[] PROGMEM = "Zone ";

#ifdef USEMETRIC
  const char TUNIT[] PROGMEM = "C";
#else
  const char TUNIT[] PROGMEM = "F";
#endif

const char MINS[] PROGMEM = "m";

//**********************************************************************************
// UI Custom LCD Chars
//**********************************************************************************
const byte CHARFIELD[] PROGMEM = {B11111, B00000, B00000, B00000, B00000, B00000, B00000, B00000};
const byte CHARCURSOR[] PROGMEM = {B11111, B11111, B00000, B00000, B00000, B00000, B00000, B00000};
const byte CHARSEL[] PROGMEM = {B10001, B11111, B00000, B00000, B00000, B00000, B00000, B00000};

const byte BMP0[] PROGMEM = {B00000, B00000, B00000, B11111, B10001, B10001, B11111, B00001};
const byte BMP1[] PROGMEM = {B00000, B00000, B00000, B00000, B00000, B00011, B01100, B01111};
const byte BMP2[] PROGMEM = {B00000, B00000, B00000, B00000, B00000, B11100, B00011, B11111};
const byte BMP3[] PROGMEM = {B00100, B01100, B01111, B00111, B00100, B01100, B01111, B00111};
const byte BMP4[] PROGMEM = {B00010, B00011, B11111, B11110, B00010, B00011, B11111, B11110};

const byte UNLOCK_ICON[] PROGMEM = {B00110, B01001, B01001, B01000, B01111, B01111, B01111, B00000};
const byte PROG_ICON[] PROGMEM = {B00001, B11101, B10101, B11101, B10001, B10001, B00001, B11111};
const byte BELL[] PROGMEM = {B00100, B01110, B01110, B01110, B11111, B00000, B00100, B00000};

//**********************************************************************************
// UI Globals
//**********************************************************************************
byte activeScreen, inMenu;
boolean screenLock;

//**********************************************************************************
// uiInit:  One time intialization of all UI logic
//**********************************************************************************
void uiInit() {
  LCD.init();
  
  #ifndef ENCODER_I2C
    #ifndef ENCODER_OLD_CONSTRUCTOR
      Encoder.begin(ENCODER_TYPE, ENTER_PIN, ENCA_PIN, ENCB_PIN);
    #else
      Encoder.begin(ENCODER_TYPE, ENTER_PIN, ENCA_PIN, ENCB_PIN, ENTER_INT, ENCA_INT);
    #endif
    #ifdef ENCODER_ACTIVELOW
      Encoder.setActiveLow(1);
    #endif
  #else
     Encoder.begin(ENCODER_I2CADDR);
  #endif

  //Check to see if EEPROM Initialization is needed
  if (checkConfig()) {
    LCD.clear();
    LCD.print_P(0, 0, PSTR("Missing Config"));
    if (confirmChoice(INIT_EEPROM, 3)) UIinitEEPROM();
    LCD.clear();
  }
}

void UIinitEEPROM() {
  LCD.clear();
  LCD.print_P(1, 0, INIT_EEPROM);
  LCD.print_P(2, 3, PSTR("Please Wait..."));
  LCD.update();
  initEEPROM();
  //Apply any EEPROM updates
  checkConfig();
}

void uiEvent(byte eventID, byte eventParam) {
  switch (eventID) {
    case EVENT_ALARM_TEMPHOT:
    case EVENT_ALARM_TEMPCOLD:
    case EVENT_ALARM_TSENSOR:
      activeScreen = eventParam + 1;
      unlockUI();
      screenInit(activeScreen);
      break;
  }
}

//**********************************************************************************
// unlockUI:  Unlock active screen to select another
//**********************************************************************************
void unlockUI() {
  Encoder.setMin(SCREEN_HOME);
  Encoder.setMax(NUM_ZONES);
  Encoder.setCount(activeScreen);
  screenLock = 0;
  //Reinit screen to show unlock icon hide parts not visible while locked
  screenInit(activeScreen);
}

void lockUI() {
  screenLock = 1;
  //Recall screenInit to setup encoder and other functions available only when locked
  screenInit(activeScreen);
}

//**********************************************************************************
// screenCore: Called in main loop to handle all UI functions
//**********************************************************************************
void uiCore() {
  if (!screenLock) {
    int encValue = Encoder.change();
    if (encValue >= 0) {
      activeScreen = encValue;
      screenInit(activeScreen);
    }
  }
  screenEnter(activeScreen);
  screenRefresh(activeScreen);
}

//**********************************************************************************
// screenInit: Initialize active screen
//**********************************************************************************
void screenInit(byte screen) {
  if (inMenu) return;
  LCD.clear();
  LCD.setCustChar_P(7, UNLOCK_ICON);
  LCD.setCustChar_P(5, BELL);
  
  if (screen == SCREEN_HOME) {
    //Screen Init: Home
    screenAbout();
    
  } else {
    LCD.print(0, 0, getZoneName(screen - 1, buf));
    
    LCD.print_P(1, 6, PSTR("Current:"));
    LCD.print_P(1, 19, TUNIT);
    LCD.print_P(2, 10,PSTR("Set:"));
    LCD.print_P(2, 19, TUNIT);
    LCD.print_P(3, 7,PSTR("Output:")); 
  }
  if (!screenLock) LCD.writeCustChar(0, 19, 7);
}

//**********************************************************************************
// screenRefresh:  Refresh active screen
//**********************************************************************************
void screenRefresh(byte screen) {
  if (isAlarmAllZones()) LCD.writeCustChar(0, 18, 5);
  else LCD.writeCustChar(0, 18, ' ');
  
  if (screen == SCREEN_HOME) {
    //Refresh Screen: Home

  } else {
    if (temp[screen - 1] == BAD_TEMP) LCD.print_P(1, 14, PSTR("-----")); else {
      vftoa(temp[screen - 1] / 10, buf, 1, 1);
      LCD.lPad(1, 14, buf, 5, ' ');
    }
    if (setpoint[screen - 1] == NO_SETPOINT) LCD.print_P(2, 14, PSTR("-----")); else {
      vftoa(setpoint[screen - 1] / 10, buf, 1, 1);
      LCD.lPad(2, 14, buf, 5, ' '); 
    }
    if (zonePwr[screen - 1] < 0) LCD.print_P(3, 16, PSTR("Cool"));
    else if (zonePwr[screen - 1] > 0) LCD.print_P(3, 16, PSTR("Heat"));
    else LCD.print_P(3, 16, PSTR(" Off"));
  }
}


//**********************************************************************************
// screenEnter:  Check enterStatus and handle based on screenLock and activeScreen
//**********************************************************************************
void screenEnter(byte screen) {
  if (Encoder.cancel()) {
    //Unlock screens
    unlockUI();
  } else if (Encoder.ok()) {
    if (!screenLock) lockUI();
    else {
      if (screen == SCREEN_HOME) {
      //Screen Enter: Home

        while(1) {
          menu homeMenu(3, 5);
          //Item updated on each cycle
          if (isAlarmAllZones()) homeMenu.setItem_P(PSTR("View Alarms"), 3);
          homeMenu.setItem_P(PSTR("System Setup"), 0);
          homeMenu.setItem_P(PSTR("Reset All Zones"), 1);
          //homeMenu.setItem_P(PSTR("About FermTroller"), 2);
          homeMenu.setItem_P(EXIT, 255);
          
          byte lastOption = scrollMenu("Main Menu", &homeMenu);
          
          if (lastOption == 0) menuSetup();
          else if (lastOption == 1) {
            //Reset All
            if (confirmAbort()) resetOutputs();
          }
          else if (lastOption == 2) {
            screenAbout();
            while (!Encoder.ok()) fermCore();
          }
          else if (lastOption == 3) menuAlarmZones();
          else {
            //On exit of the Main menu go back to Splash/Home screen.
            activeScreen = SCREEN_HOME;
            screenInit(activeScreen);
            unlockUI();
            break;
          }
        }
      } else {
        while(1) {
          char title[18];
          getZoneName(screen - 1, title);
          
          menu zoneMenu(3, 9);
          if (isAlarmAllZones()) zoneMenu.setItem_P(PSTR("View Alarms"), 0);
          zoneMenu.setItem_P(PSTR("Setpoint: "), 1);
          if (setpoint[screen - 1] == NO_SETPOINT) zoneMenu.appendItem_P(PSTR("N/A"), 1);
          else {
            vftoa(setpoint[screen - 1] / 10, buf, 1, 1); 
            zoneMenu.appendItem(buf, 1);
            zoneMenu.appendItem_P(TUNIT, 1);
          }
          if (setpoint[screen - 1] != NO_SETPOINT) zoneMenu.setItem_P(PSTR("Clear Setpoint"), 2);
          zoneMenu.setItem_P(PSTR("Zone Name"), 3);

          zoneMenu.setItem_P(PSTR("Hysteresis: "), 4);
          vftoa(hysteresis[screen - 1], buf, 1, 1);
          zoneMenu.appendItem(buf, 4);
          zoneMenu.appendItem_P(TUNIT, 4);
          
          zoneMenu.setItem_P(PSTR("Alarm Thresh: "), 5);
          vftoa(alarmThresh[screen - 1], buf, 1, 1);
          zoneMenu.appendItem(buf, 5);
          zoneMenu.appendItem_P(TUNIT, 5);

          zoneMenu.setItem_P(EXIT, 255);

          byte lastOption = scrollMenu(title, &zoneMenu);
          if (lastOption == 0) menuAlarmZones();
          else if (lastOption == 1) {
            long newValue = getValue(title, (setpoint[screen - 1] == NO_SETPOINT ? 0 : setpoint[screen - 1] / 10), 1, SETPOINT_MIN / 10, SETPOINT_MAX / 10, TUNIT);
            if (newValue != GETVALUE_CANCEL) setSetpoint(screen - 1, newValue * 10);
          }
          else if (lastOption == 2) setSetpoint(screen - 1, NO_SETPOINT);
          else if (lastOption == 3) {
            getString(PSTR("Zone Name:"), title, 17);
            setZoneName(screen - 1, title);
          }
          else if (lastOption == 4) {
            long newValue = getValue_P(PSTR("Hysteresis"), hysteresis[screen - 1], 1, 0, 255, TUNIT);
            if (newValue != GETVALUE_CANCEL) setHysteresis(screen - 1, newValue);
          }
          else if (lastOption == 5) {
            long newValue = getValue_P(PSTR("Alarm Thresh"), alarmThresh[screen - 1], 1, 0, 255, TUNIT);
            if (newValue != GETVALUE_CANCEL) setAlarmThresh(screen - 1, newValue);
          }
          else break;
        }
      }
      screenInit(activeScreen);
    }
  }
}

void screenAbout() {
  LCD.clear();
  LCD.setCustChar_P(0, BMP0);
  LCD.setCustChar_P(1, BMP1);
  LCD.setCustChar_P(2, BMP2);
  LCD.setCustChar_P(3, BMP3);
  LCD.setCustChar_P(4, BMP4);
  LCD.writeCustChar(0, 0, 0);
  LCD.writeCustChar(0, 1, 1);
  LCD.writeCustChar(0, 2, 2);
  LCD.writeCustChar(1, 1, 3);
  LCD.writeCustChar(1, 2, 4);
  LCD.print_P(1, 4, BT);
  LCD.print_P(1, 16, BTVER);
  LCD.print_P(2, 4, PSTR("Build"));
  LCD.lPad(2, 10, itoa(BUILDNUM, buf, 10), 4, '0');
  LCD.print_P(3, 0, PSTR("www.brewtroller.com"));
  LCD.update();
}

void splashScreen() { 
  //screenAbout();
  //while (millis() < 5000 && !Encoder.ok()) fermCore; 
}

void menuAlarmZones() {
  while (1) {
    menu alarmZonesMenu(3, NUM_ZONES * 2);
    for (byte zone = 0; zone < NUM_ZONES; zone++) {
      if (alarmStatus[zone]) alarmZonesMenu.setItem(getZoneName(zone, buf), zone);
    }
    if (!alarmZonesMenu.getItemCount()) return;
    alarmZonesMenu.setItem_P(EXIT, 255);
    byte lastOption = scrollMenu("Zone Alarms", &alarmZonesMenu);
    if (lastOption < NUM_ZONES) menuAlarms(lastOption);
    else return;
  }
}

void menuAlarms(byte zone) {
  char title[18];
  getZoneName(zone, title);
  while (1) {
    menu alarmMenu(3, ALARM_COUNT);
    if (bitRead(alarmStatus[zone], ALARM_STATUS_TSENSOR) && bitRead(alarmStatus[zone], ALARM_ACK_TSENSOR)) alarmMenu.setItem_P(PSTR("TSensor (Active)"), ALARM_ACK_TSENSOR);         //Unack'ed Current Alarm
    else if (!bitRead(alarmStatus[zone], ALARM_STATUS_TSENSOR) && bitRead(alarmStatus[zone], ALARM_ACK_TSENSOR)) alarmMenu.setItem_P(PSTR("TSensor"), ALARM_ACK_TSENSOR); //Unack'd Historic Alarm
    else if (bitRead(alarmStatus[zone], ALARM_STATUS_TSENSOR) && !bitRead(alarmStatus[zone], ALARM_ACK_TSENSOR)) alarmMenu.setItem_P(PSTR("TSensor (Ack)"), ALARM_ACK_TSENSOR);   //Unack'ed Current Alarm//Ack'd Current Alarm

    if (bitRead(alarmStatus[zone], ALARM_STATUS_TEMPHOT) && bitRead(alarmStatus[zone], ALARM_ACK_TEMPHOT)) alarmMenu.setItem_P(PSTR("Temp High (Active)"), ALARM_ACK_TEMPHOT);         //Unack'ed Current Alarm
    else if (!bitRead(alarmStatus[zone], ALARM_STATUS_TEMPHOT) && bitRead(alarmStatus[zone], ALARM_ACK_TEMPHOT)) alarmMenu.setItem_P(PSTR("Temp High"), ALARM_ACK_TEMPHOT); //Unack'd Historic Alarm
    else if (bitRead(alarmStatus[zone], ALARM_STATUS_TEMPHOT) && !bitRead(alarmStatus[zone], ALARM_ACK_TEMPHOT)) alarmMenu.setItem_P(PSTR("Temp High (Ack)"), ALARM_ACK_TEMPHOT);   //Unack'ed Current Alarm//Ack'd Current Alarm

    if (bitRead(alarmStatus[zone], ALARM_STATUS_TEMPCOLD) && bitRead(alarmStatus[zone], ALARM_ACK_TEMPCOLD)) alarmMenu.setItem_P(PSTR("Temp Low (Active)"), ALARM_ACK_TEMPCOLD);         //Unack'ed Current Alarm
    else if (!bitRead(alarmStatus[zone], ALARM_STATUS_TEMPCOLD) && bitRead(alarmStatus[zone], ALARM_ACK_TEMPCOLD)) alarmMenu.setItem_P(PSTR("Temp Low"), ALARM_ACK_TEMPCOLD); //Unack'd Historic Alarm
    else if (bitRead(alarmStatus[zone], ALARM_STATUS_TEMPCOLD) && !bitRead(alarmStatus[zone], ALARM_ACK_TEMPCOLD)) alarmMenu.setItem_P(PSTR("Temp Low (Ack)"), ALARM_ACK_TEMPCOLD);   //Unack'ed Current Alarm//Ack'd Current Alarm
    if (!alarmMenu.getItemCount()) return;
    alarmMenu.setItem_P(EXIT, 255);
    
    byte lastOption = scrollMenu(title, &alarmMenu);
    if (lastOption != 255) {
      bitClear(alarmStatus[zone], lastOption); //ACK the Alarm by clearing the ACK Req'd Bit
      saveAlarmStatus(zone);
    }
    else return;
  }  
}


//*****************************************************************************************************************************
// System Setup Menus
//*****************************************************************************************************************************
void menuSetup() {
  menu setupMenu(3, 6);
  #ifdef UI_DISPLAY_SETUP
    setupMenu.setItem_P(PSTR("Display"), 3);
  #endif
  setupMenu.setItem_P(PSTR("Temperature Sensors"), 0);
  setupMenu.setItem_P(PSTR("Manual Control"), 1); 
  setupMenu.setItem_P(PSTR("Outputs"), 2);
  setupMenu.setItem_P(INIT_EEPROM, 3);

  setupMenu.setItem_P(EXIT, 255);
  
  while(1) {
    byte lastOption = scrollMenu("System Setup", &setupMenu);
    if (lastOption == 0) assignSensor();
    else if (lastOption == 1) uiManual();
    else if (lastOption == 2) cfgOutputs();
    else if (lastOption == 3) {
      LCD.clear();
      LCD.print_P(0, 0, PSTR("Reset Configuration?"));
      if (confirmChoice(INIT_EEPROM, 3)) UIinitEEPROM();
    }
    #ifdef UI_DISPLAY_SETUP
      else if (lastOption == 3) adjustLCD();
    #endif
    else return;
  }
}

void assignSensor() {
  inMenu++;
  menu tsMenu(1, NUM_ZONES);
  for (byte zone = 0; zone < NUM_ZONES; zone++) {
    tsMenu.setItem_P(ZONE, zone);
    itoa(zone + 1, buf, 10);
    tsMenu.appendItem(buf, zone);
  }

  Encoder.setMin(0);
  Encoder.setMax(NUM_ZONES - 1);
  Encoder.setCount(0);
  
  boolean redraw = 1;
  int encValue;
  
  while (1) {
    if (redraw) {
      //First time entry or back from the sub-menu.
      redraw = 0;
      encValue = Encoder.getCount();
    } else encValue = Encoder.change();
    
    if (encValue >= 0) {
      tsMenu.setSelected(encValue);
      //The user has navigated toward a new temperature probe screen.
      LCD.clear();
      LCD.print_P(0, 0, PSTR("Assign Temp Sensor"));
      LCD.center(1, 0, tsMenu.getSelectedRow(buf), 20);
      for (byte i = 0; i < 8; i++) LCD.lPad(2, i * 2 + 2, itoa(tSensor[encValue][i], buf, 16), 2, '0');
    }
    displayAssignSensorTemp(tsMenu.getValue()); //Update each loop

    if (Encoder.cancel()) {
      inMenu--;
      return;
    }
    else if (Encoder.ok()) {
      encValue = Encoder.getCount();
      //Pop-Up Menu
      menu tsOpMenu(3, 4);
      tsOpMenu.setItem_P(PSTR("Scan Bus"), 0);
      tsOpMenu.setItem_P(PSTR("Delete Address"), 1);
      tsOpMenu.setItem_P(CANCEL, 2);
      tsOpMenu.setItem_P(EXIT, 255);
      byte selected = scrollMenu(tsMenu.getSelectedRow(buf), &tsOpMenu);
      if (selected == 0) {
        LCD.clear();
        LCD.center(0, 0, tsMenu.getSelectedRow(buf), 20);
        LCD.print_P(1,0,PSTR("Disconnect all other"));
        LCD.print_P(2,2,PSTR("temp sensors now"));
        {
          if (confirmChoice(CONTINUE, 3)) {
            byte addr[8] = {0, 0, 0, 0, 0, 0, 0, 0};
            getDSAddr(addr);
            setTSAddr(encValue, addr);
          }
        }
      } else if (selected == 1) {
        byte addr[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        setTSAddr(encValue, addr);
      }
      else if (selected > 2) {
        inMenu--;
        return;
      }
      
      Encoder.setMin(0);
      Encoder.setMax(tsMenu.getItemCount() - 1);
      Encoder.setCount(tsMenu.getSelected());
      redraw = 1;
    }
    fermCore();
  }
}

void displayAssignSensorTemp(int sensor) {
  LCD.print_P(3, 10, TUNIT); 
  if (temp[sensor] == -32768) {
    LCD.print_P(3, 7, PSTR("---"));
  } else {
    LCD.lPad(3, 7, itoa(temp[sensor] / 100, buf, 10), 3, ' ');
  }
}

void uiManual() {
  menu manualMenu(3, NUM_ZONES + 1);
  
  for (byte zone = 0; zone < NUM_ZONES; zone++) {
    manualMenu.setItem_P(ZONE, zone);
    itoa(zone + 1, buf, 10);
    manualMenu.appendItem(buf, zone);
  }
  manualMenu.setItem_P(EXIT, 255);

  while(1) {    
    byte zone = scrollMenu("Manual Settings", &manualMenu);
    if (zone < NUM_ZONES) cfgManual(zone, manualMenu.getSelectedRow(buf));
    else return;
    fermCore();
  } 
}

void cfgManual(byte zone, char sTitle[]) {
  menu outputMenu(3, 5);
  while(1) { 
    outputMenu.setItem_P(PSTR("Manual Control:"), 1);
    if (manualControl[zone])
      outputMenu.appendItem("On", 1);
    else
      outputMenu.appendItem("Off", 1);  
    
    outputMenu.setItem_P(PSTR("On Time: "), 2);
    byte hours = coolMinOn[zone] / 60;
    outputMenu.appendItem(itoa(hours, buf, 10), 2);
    outputMenu.appendItem(":", 2);
    byte mins = coolMinOn[zone] - hours * 60;
    itoa(mins, buf, 10);
    strLPad(buf, 2, '0');
    outputMenu.appendItem(buf, 2);
    
    outputMenu.setItem_P(PSTR("Off Time: "), 3);
    hours = coolMinOff[zone] / 60;
    outputMenu.appendItem(itoa(hours, buf, 10), 3);
    outputMenu.appendItem(":", 3);
    mins = coolMinOff[zone] - hours * 60;
    itoa(mins, buf, 10);
    strLPad(buf, 2, '0');
    outputMenu.appendItem(buf, 3);

    outputMenu.setItem_P(EXIT, 255);
 
    byte lastOption = scrollMenu(sTitle, &outputMenu);
    char hTitle[21];
    strcpy(hTitle, sTitle);

    if (lastOption == 1) setManualControl(zone, manualControl[zone] ? 0 : 1);
    else if (lastOption == 2) setCoolMinOn(zone, getTimerValue(PSTR("Min Cool On"), coolMinOn[zone], 4));
    else if (lastOption == 3) setCoolMinOff(zone, getTimerValue(PSTR("Min Cool Off"), coolMinOff[zone], 4));
    else return;
    fermCore();
  } 
}

void cfgOutputs() {
  menu outputsMenu(3, NUM_ZONES + 2);
  
  for (byte zone = 0; zone < NUM_ZONES; zone++) {
    outputsMenu.setItem_P(ZONE, zone);
    itoa(zone + 1, buf, 10);
    outputsMenu.appendItem(buf, zone);
  }
  outputsMenu.setItem_P(PSTR("Alarm"), NUM_ZONES);
  outputsMenu.setItem_P(EXIT, 255);

  while(1) {    
    byte zone = scrollMenu("Output Settings", &outputsMenu);
    if (zone < NUM_ZONES) cfgOutput(zone, outputsMenu.getSelectedRow(buf));
    else if (zone == NUM_ZONES) setValveCfg(VLV_ALARM, cfgValveProfile("Alarm", vlvConfig[VLV_ALARM]));
    else return;
    fermCore();
  } 
}

void cfgOutput(byte zone, char sTitle[]) {
  menu outputMenu(3, 6);
  while(1) { 
    outputMenu.setItem_P(PSTR("Heat Output Profile"), 2);
    outputMenu.setItem_P(PSTR("Cool Output Profile"), 3);
    
    outputMenu.setItem_P(PSTR("Min Cool On: "), 4);
    byte hours = coolMinOn[zone] / 60;
    outputMenu.appendItem(itoa(hours, buf, 10), 4);
    outputMenu.appendItem(":", 4);
    byte mins = coolMinOn[zone] - hours * 60;
    itoa(mins, buf, 10);
    strLPad(buf, 2, '0');
    outputMenu.appendItem(buf, 4);
    
    outputMenu.setItem_P(PSTR("Min Cool Off: "), 5);
    hours = coolMinOff[zone] / 60;
    outputMenu.appendItem(itoa(hours, buf, 10), 5);
    outputMenu.appendItem(":", 5);
    mins = coolMinOff[zone] - hours * 60;
    itoa(mins, buf, 10);
    strLPad(buf, 2, '0');
    outputMenu.appendItem(buf, 5);

    outputMenu.setItem_P(PSTR("Max Cool On: "), 6);
    hours = coolMaxOn[zone] / 60;
    outputMenu.appendItem(itoa(hours, buf, 10), 6);
    outputMenu.appendItem(":", 6);
    mins = coolMaxOn[zone] - hours * 60;
    itoa(mins, buf, 10);
    strLPad(buf, 2, '0');
    outputMenu.appendItem(buf, 6);

    outputMenu.setItem_P(EXIT, 255);
 
    byte lastOption = scrollMenu(sTitle, &outputMenu);
    char hTitle[21];
    strcpy(hTitle, sTitle);

    if (lastOption == 2) {
      strcat_P(hTitle, PSTR(" Heat"));
      setValveCfg(zone, cfgValveProfile(hTitle, vlvConfig[zone]));
    }
    else if (lastOption == 3) {
      strcat_P(hTitle, PSTR(" Cool"));
      setValveCfg(NUM_ZONES + zone, cfgValveProfile(hTitle, vlvConfig[NUM_ZONES + zone]));
    }
    else if (lastOption == 4) setCoolMinOn(zone, getTimerValue(PSTR("Min Cool On"), coolMinOn[zone], 4));
    else if (lastOption == 5) setCoolMinOff(zone, getTimerValue(PSTR("Min Cool Off"), coolMinOff[zone], 4));
    else if (lastOption == 6) setCoolMaxOn(zone, getTimerValue(PSTR("Max Cool On"), coolMaxOn[zone], 4));
    else return;
    fermCore();
  } 
}


unsigned long cfgValveProfile (const char *sTitle, unsigned long defValue) {
  unsigned long retValue = defValue;
  //firstBit: The left most bit being displayed
  byte firstBit, encMax;
  inMenu++;
  
  encMax = PVOUT_COUNT + 1;

  Encoder.setMin(0);
  Encoder.setCount(0);
  Encoder.setMax(encMax);
  //(Set to MAX + 1 to force redraw)
  firstBit = encMax + 1;
  
  LCD.clear();
  LCD.print(0,0,sTitle);
  LCD.print_P(3, 3, PSTR("Test"));
  LCD.print_P(3, 13, PSTR("Save"));
  
  boolean redraw = 1;
  while(1) {
    int encValue;
    if (redraw) {
      redraw = 0;
      encValue = Encoder.getCount();
    }
    else encValue = Encoder.change();
    if (encValue >= 0) {
      if (encValue < firstBit || encValue > firstBit + 17) {
        if (encValue < firstBit) firstBit = encValue; else if (encValue < encMax - 1) firstBit = encValue - 17;
        for (byte i = firstBit; i < min(encMax - 1, firstBit + 18); i++) if (retValue & ((unsigned long)1<<i)) LCD.print_P(1, i - firstBit + 1, PSTR("1")); else LCD.print_P(1, i - firstBit + 1, PSTR("0"));
      }

      for (byte i = firstBit; i < min(encMax - 1, firstBit + 18); i++) {
        if (i < 9) itoa(i + 1, buf, 10); else buf[0] = i + 56;
        buf[1] = '\0';
        LCD.print(2, i - firstBit + 1, buf);
      }

      if (firstBit > 0) LCD.print_P(2, 0, PSTR("<")); else LCD.print_P(2, 0, PSTR(" "));
      if (firstBit + 18 < encMax - 1) LCD.print_P(2, 19, PSTR(">")); else LCD.print_P(2, 19, PSTR(" "));
      if (encValue == encMax - 1) {
        LCD.print_P(3, 2, PSTR(">"));
        LCD.print_P(3, 7, PSTR("<"));
        LCD.print_P(3, 12, PSTR(" "));
        LCD.print_P(3, 17, PSTR(" "));
      } else if (encValue == encMax) {
        LCD.print_P(3, 2, PSTR(" "));
        LCD.print_P(3, 7, PSTR(" "));
        LCD.print_P(3, 12, PSTR(">"));
        LCD.print_P(3, 17, PSTR("<"));
      } else {
        LCD.print_P(3, 2, PSTR(" "));
        LCD.print_P(3, 7, PSTR(" "));
        LCD.print_P(3, 12, PSTR(" "));
        LCD.print_P(3, 17, PSTR(" "));
        LCD.print_P(2, encValue - firstBit + 1, PSTR("^"));
      }
    }
    
    if (Encoder.ok()) {
      encValue = Encoder.getCount();
      if (encValue == encMax) {
        inMenu--;
        return retValue;
      }
      else if (encValue == encMax - 1) {
        Valves.set(retValue);
        LCD.print_P(3, 2, PSTR("["));
        LCD.print_P(3, 7, PSTR("]"));
        LCD.update();
        while (!Encoder.ok()) delay(100);
        Valves.set(0);
        redraw = 1;
      } else {
        retValue = retValue ^ ((unsigned long)1<<encValue);
        for (byte i = firstBit; i < min(encMax - 1, firstBit + 18); i++) if (retValue & ((unsigned long)1<<i)) LCD.print_P(1, i - firstBit + 1, PSTR("1")); else LCD.print_P(1, i - firstBit + 1, PSTR("0"));
      }
    } else if (Encoder.cancel()) {
      inMenu--;
      return defValue;
    }
    fermCore();
  }
}

#ifdef UI_DISPLAY_SETUP
  void adjustLCD() {
    byte cursorPos = 0; //0 = brightness, 1 = contrast, 2 = cancel, 3 = save
    boolean cursorState = 0; //0 = Unselected, 1 = Selected
    inMenu++;
    
    Encoder.setMin(0);
    Encoder.setCount(0);
    Encoder.setMax(3);
    
    LCD.clear();
    LCD.print_P(0,0,PSTR("Adjust LCD"));
    LCD.print_P(1, 1, PSTR("Brightness:"));
    LCD.print_P(2, 3, PSTR("Contrast:"));
    LCD.print_P(3, 1, PSTR("Cancel"));
    LCD.print_P(3, 15, PSTR("Save"));
    byte bright = LCD.getBright();
    byte contrast = LCD.getContrast();
    byte origBright = bright;
    byte origContrast = contrast;
    boolean redraw = 1;
    while(1) {
      int encValue;
      if (redraw) {
        redraw = 0;
        encValue = Encoder.getCount();
      }
      else encValue = Encoder.change();
      if (encValue >= 0) {
        if (cursorState) {
          if (cursorPos == 0) { 
            bright = encValue;
            LCD.setBright(bright);
          } else if (cursorPos == 1) {
            contrast = encValue;
            LCD.setContrast(contrast);
          }
        } else {
          cursorPos = encValue;
          LCD.print_P(1, 12, PSTR(" "));
          LCD.print_P(1, 16, PSTR(" "));
          LCD.print_P(2, 12, PSTR(" "));
          LCD.print_P(2, 16, PSTR(" "));
          LCD.print_P(3, 0, PSTR(" "));
          LCD.print_P(3, 7, PSTR(" "));
          LCD.print_P(3, 14, PSTR(" "));
          LCD.print_P(3, 19, PSTR(" "));
          if (cursorPos == 0) {
            LCD.print_P(1, 12, PSTR(">"));
            LCD.print_P(1, 16, PSTR("<"));
          } else if (cursorPos == 1) {
            LCD.print_P(2, 12, PSTR(">"));
            LCD.print_P(2, 16, PSTR("<"));
          } else if (cursorPos == 2) {
            LCD.print_P(3, 0, PSTR(">"));
            LCD.print_P(3, 7, PSTR("<"));
          } else if (cursorPos == 3) {
            LCD.print_P(3, 14, PSTR(">"));
            LCD.print_P(3, 19, PSTR("<"));
          }
        }
        LCD.lPad(1, 13, itoa(bright, buf, 10), 3, ' ');
        LCD.lPad(2, 13, itoa(contrast, buf, 10), 3, ' ');
      }
      if (Encoder.ok()) {
        if (cursorPos == 2) {
          LCD.setBright(origBright);
          LCD.setContrast(origContrast);
          inMenu--;
          return;
        }
        else if (cursorPos == 3) {
          LCD.saveConfig();
          inMenu--;
          return;
        }
        cursorState = cursorState ^ 1;
        if (cursorState) {
          Encoder.setMin(0);
          Encoder.setMax(255);
          if (cursorPos == 0) Encoder.setCount(bright);
          else if (cursorPos == 1) Encoder.setCount(contrast);
        } else {
          Encoder.setMin(0);
          Encoder.setMax(3);
          Encoder.setCount(cursorPos);
        }
      } else if (Encoder.cancel()) {
        inMenu--;
        return;
      }
      fermCore();
    }
  }
#endif //#ifdef UI_DISPLAY_SETUP



//*****************************************************************************************************************************
//Generic Menu Functions
//*****************************************************************************************************************************
/*
  scrollMenu() & drawMenu():
  Glues together menu, Encoder and LCD objects
*/

byte scrollMenu(const char* sTitle, menu *objMenu) {
  inMenu++;
  Encoder.setMin(0);
  Encoder.setMax(objMenu->getItemCount() - 1);
  //Force refresh in case selected value was set
  Encoder.setCount(objMenu->getSelected());
  boolean redraw = 1;
  
  while(1) {
    int encValue;
    if (redraw) encValue = Encoder.getCount();
    else encValue = Encoder.change();
    if (encValue >= 0) {
      objMenu->setSelected(Encoder.getCount());
      if (objMenu->refreshDisp() || redraw) drawMenu(sTitle, objMenu);
      for (byte i = 0; i < 3; i++) LCD.print(i + 1, 0, " ");
      LCD.print(objMenu->getCursor() + 1, 0, ">");
    }
    redraw = 0;
    //If Enter
    if (Encoder.ok()) {
      inMenu--;
      return objMenu->getValue();
    } else if (Encoder.cancel()) {
      inMenu--;
      return 255;
    }
    fermCore();
  }
}

void drawMenu(const char* sTitle, menu *objMenu) {
  LCD.clear();
  if (sTitle != NULL) LCD.print(0, 0, sTitle);

  for (byte i = 0; i < 3; i++) {
    objMenu->getVisibleRow(i, buf);
    LCD.print(i + 1, 1, buf);
  }
  LCD.print(objMenu->getCursor() + 1, 0, ">");
}

byte getChoice(menu *objMenu, byte iRow) {
  inMenu++;
  LCD.print_P(iRow, 0, PSTR(">"));
  LCD.print_P(iRow, 19, PSTR("<"));
  Encoder.setMin(0);
  Encoder.setMax(objMenu->getItemCount() - 1);
  Encoder.setCount(0);
  boolean redraw = 1;
  
  while(1) {
    int encValue;
    if (redraw) {
      redraw = 0;
      encValue = Encoder.getCount();
    }
    else encValue = Encoder.change();
    if (encValue >= 0) {
      objMenu->setSelected(encValue);
      LCD.center(iRow, 1, objMenu->getSelectedRow(buf), 18);
    }
    
    //If Enter
    if (Encoder.ok()) {
      LCD.print_P(iRow, 0, SPACE);
      LCD.print_P(iRow, 19, SPACE);
      inMenu--;
      return Encoder.getCount();
    } else if (Encoder.cancel()) {
      inMenu--;
      return 255;
    }
    fermCore();
  }
}

boolean confirmChoice(const char *choice, byte row) {
  menu choiceMenu(1, 2);
  choiceMenu.setItem_P(CANCEL, 0);
  choiceMenu.setItem_P(choice, 1);
  if(getChoice(&choiceMenu, row) == 1) return 1; else return 0;
}

boolean confirmAbort() {
  LCD.clear();
  LCD.print_P(0, 0, PSTR("Abort operation and"));
  LCD.print_P(1, 0, PSTR("reset setpoints,"));
  LCD.print_P(2, 0, PSTR("timers and outputs?"));
  return confirmChoice(PSTR("Reset"), 3);
}

boolean confirmDel() {
  LCD.clear();
  LCD.print_P(1, 0, PSTR("Delete Item?"));
  return confirmChoice(PSTR("Delete"), 3);
}

long getValue_P(const char *sTitle, long defValue, byte precision, long minValue, long maxValue, const char *dispUnit) {
  char title[20];
  strcpy_P(title, sTitle);
  return getValue(title, defValue, precision, minValue, maxValue, dispUnit);
}

long getValue(char sTitle[], long defValue, byte precision, long minValue, long maxValue, const char *dispUnit) {
  long retValue = constrain(defValue, minValue, maxValue);
  byte cursorPos = 0; 
  boolean cursorState = 0; //0 = Unselected, 1 = Selected
  char strValue[12];
  boolean sign;
  
  inMenu++;
  sign = (minValue < 0 ? 1 : 0);
  unsigned int mult = pow10(precision);
  long bigVal = max(abs(minValue), maxValue);
  ltoa(bigVal/mult, strValue, 10);
  byte digits = strlen(strValue) + precision;
  if (sign) digits++;

  Encoder.setMin(0);
  Encoder.setMax(digits);
  Encoder.setCount(0);

  LCD.setCustChar_P(0, CHARFIELD);
  LCD.setCustChar_P(1, CHARCURSOR);
  LCD.setCustChar_P(2, CHARSEL);
  
  byte fieldStart = (20 - digits + 1) / 2;
  LCD.clear();
  LCD.print(0, 0, sTitle);
  LCD.print_P(1, fieldStart + digits + 1, dispUnit);
  LCD.print_P(3, 9, OK);
  boolean redraw = 1;

  while(1) {
    int encValue;
    if (redraw) {
      redraw = 0;
      encValue = Encoder.getCount();
    }
    else encValue = Encoder.change();
    if (encValue >= 0) {
      if (cursorState) {
        if (sign && cursorPos == 0) {
          //Handle Sign position
          if (encValue) retValue = abs(retValue);
          else retValue = 0 - abs(retValue);
        }
        else {
          //Handle digit position
          ltoa(abs(retValue), strValue, 10);
          strLPad(strValue, digits - (sign ? 1 : 0), '0');
          strValue[cursorPos - (sign ? 1 : 0)] = '0' + encValue;
          if (retValue < 0) retValue = 0 - atol(strValue);
          else retValue = atol(strValue);
        }
        retValue = constrain(retValue, minValue, maxValue);
      } else {
        cursorPos = encValue;
        for (byte i = fieldStart - 1; i < fieldStart - 1 + digits - precision; i++) LCD.writeCustChar(2, i, 0);
        if (precision) for (byte i = fieldStart + digits - precision; i < fieldStart + digits; i++) LCD.writeCustChar(2, i, 0);
        LCD.print(3, 8, " ");
        LCD.print(3, 11, " ");
        if (cursorPos == digits) {
          LCD.print(3, 8, ">");
          LCD.print(3, 11, "<");
        } else {
          if (cursorPos < digits - precision) LCD.writeCustChar(2, fieldStart + cursorPos - 1, 1);
          else LCD.writeCustChar(2, fieldStart + cursorPos, 1);
        }
      }
      if (sign) { if (retValue < 0) LCD.print(1, fieldStart - 1, "-"); else LCD.print(1, fieldStart - 1, " "); }
      vftoa(abs(retValue), strValue, precision, 1);
      strLPad(strValue, digits + (precision ? 1 : 0) - (sign ? 1 : 0), ' ');
      LCD.print(1, fieldStart - (sign ? 0 : 1), strValue);
    }
    
    if (Encoder.ok()) {
      if (cursorPos == digits) break;
      else {
        cursorState = cursorState ^ 1;
        if (cursorState) {
          if (cursorPos < digits - precision) LCD.writeCustChar(2, fieldStart + cursorPos - 1, 2);
          else LCD.writeCustChar(2, fieldStart + cursorPos, 2);
          
          if (sign && cursorPos == 0) {
            //Handle Sign
            Encoder.setMin(0);
            Encoder.setMax(1);
            if (retValue < 0) Encoder.setCount(0);
            else Encoder.setCount(1);
          }
          else {
            Encoder.setMin(0);
            Encoder.setMax(9);
            vftoa(abs(retValue), strValue, precision, 0);
            strLPad(strValue, digits - (sign ? 1 : 0), '0');
            Encoder.setCount(strValue[cursorPos - (sign ? 1 : 0)] - '0');
          }
          
        } else {
          if (cursorPos < digits - precision) LCD.writeCustChar(2, fieldStart + cursorPos - 1, 1);
          else LCD.writeCustChar(2, fieldStart + cursorPos, 1);
          Encoder.setMin(0);
          Encoder.setMax(digits);
          Encoder.setCount(cursorPos);
        }
      }
    } else if (Encoder.cancel()) {
      retValue = GETVALUE_CANCEL;
      break;
    }
    fermCore();
  }
  inMenu--;
  return retValue;
}

int getTimerValue(const char *sTitle, int defMins, byte maxHours) {
  byte hours = defMins / 60;
  byte mins = defMins - hours * 60;
  byte cursorPos = 0; //0 = Hours, 1 = Mins, 2 = OK
  boolean cursorState = 0; //0 = Unselected, 1 = Selected
  Encoder.setMin(0);
  Encoder.setMax(2);
  Encoder.setCount(0);
  
  LCD.clear();
  LCD.print_P(0,0,sTitle);
  LCD.print(1, 7, "(hh:mm)");
  LCD.print(2, 10, ":");
  LCD.print_P(3, 9, OK);
  boolean redraw = 1;
  int encValue;
 
  while(1) {
    if (redraw) {
      redraw = 0;
      encValue = Encoder.getCount();
    } else encValue = Encoder.change();
    if (encValue >= 0) {
      if (cursorState) {
        if (cursorPos) mins = encValue; else hours = encValue;
      } else {
        cursorPos = encValue;
        switch (cursorPos) {
          case 0: //hours
            LCD.print(2, 7, ">");
            LCD.print(2, 13, " ");
            LCD.print(3, 8, " ");
            LCD.print(3, 11, " ");
            break;
          case 1: //mins
            LCD.print(2, 7, " ");
            LCD.print(2, 13, "<");
            LCD.print(3, 8, " ");
            LCD.print(3, 11, " ");
            break;
          case 2: //OK
            LCD.print(2, 7, " ");
            LCD.print(2, 13, " ");
            LCD.print(3, 8, ">");
            LCD.print(3, 11, "<");
            break;
        }
      }
      LCD.lPad(2, 8, itoa(hours, buf, 10), 2, '0');
      LCD.lPad(2, 11, itoa(mins, buf, 10), 2, '0');
    }
    
    if (Encoder.ok()) {
      if (cursorPos == 2) return hours * 60 + mins;
      cursorState = cursorState ^ 1; //Toggles between value editing mode and cursor navigation.
      if (cursorState) {
        //Edition mode
        Encoder.setMin(0);
        if (cursorPos) {
          //Editing minutes
          Encoder.setMax(59);
          Encoder.setCount(mins); 
        } else {
          //Editing hours
          Encoder.setMax(maxHours);
          Encoder.setCount(hours);
        }
      } else {
        Encoder.setMin(0);
        Encoder.setMax(2);
        Encoder.setCount(cursorPos);
      }
    } else if (Encoder.cancel()) return -1; //This value will be validated in SetTimerValue. SetTimerValue will reject the storage of the timer value. 
    fermCore();
  }
}

void getString(const char *sTitle, char defValue[], byte chars) {
  char retValue[20];
  strcpy(retValue, defValue);
  inMenu++;
  
  //Right-Pad with spaces
  boolean doWipe = 0;
  for (byte i = 0; i < chars; i++) {
    if (retValue[i] < 32 || retValue[i] > 126) doWipe = 1;
    if (doWipe) retValue[i] = 32;
  }
  retValue[chars] = '\0';
  
  byte cursorPos = 0; 
  boolean cursorState = 0; //0 = Unselected, 1 = Selected
  Encoder.setMin(0);
  Encoder.setMax(chars);
  Encoder.setCount(0);


  LCD.setCustChar_P(0, CHARFIELD);
  LCD.setCustChar_P(1, CHARCURSOR);
  LCD.setCustChar_P(2, CHARSEL);
  
  LCD.clear();
  LCD.print_P(0,0,sTitle);
  LCD.print_P(3, 9, OK);
  boolean redraw = 1;
  while(1) {
    int encValue;
    if (redraw) {
      redraw = 0;
      encValue = Encoder.getCount();
    }
    else encValue = Encoder.change();
    if (encValue >= 0) {
      if (cursorState) {
        retValue[cursorPos] = enc2ASCII(encValue);
      } else {
        cursorPos = encValue;
        for (byte i = (20 - chars + 1) / 2 - 1; i < (20 - chars + 1) / 2 - 1 + chars; i++) LCD.writeCustChar(2, i, 0);
        LCD.print(3, 8, " ");
        LCD.print(3, 11, " ");
        if (cursorPos == chars) {
          LCD.print(3, 8, ">");
          LCD.print(3, 11, "<");
        } else {
          LCD.writeCustChar(2, (20 - chars + 1) / 2 + cursorPos - 1, 1);
        }
      }
      LCD.print(1, (20 - chars + 1) / 2 - 1, retValue);
    }
    
    if (Encoder.ok()) {
      if (cursorPos == chars) {
        strcpy(defValue, retValue);
        inMenu--;
        return;
      }
      else {
        cursorState = cursorState ^ 1;
        if (cursorState) {
          Encoder.setMin(0);
          Encoder.setMax(94);
          Encoder.setCount(ASCII2enc(retValue[cursorPos]));
          LCD.writeCustChar(2, (20 - chars + 1) / 2 + cursorPos - 1, 2);
        } else {
          Encoder.setMin(0);
          Encoder.setMax(chars);
          Encoder.setCount(cursorPos);
          LCD.writeCustChar(2, (20 - chars + 1) / 2 + cursorPos - 1, 1);
        }
      }
    } else if (Encoder.cancel()) {
      inMenu--;
      return;
    }
    fermCore();
  }
}

//Next two functions used to change order of charactor scroll to (space), A-Z, a-z, 0-9, symbols
byte ASCII2enc(byte charin) {
  if (charin == 32) return 0;
  else if (charin >= 65 && charin <= 90) return charin - 64;
  else if (charin >= 97 && charin <= 122) return charin - 70;
  else if (charin >= 48 && charin <= 57) return charin + 5;
  else if (charin >= 33 && charin <= 47) return charin + 30;
  else if (charin >= 58 && charin <= 64) return charin + 20;
  else if (charin >= 91 && charin <= 96) return charin - 6;
  else if (charin >= 123 && charin <= 126) return charin - 32;
}

byte enc2ASCII(byte charin) {
  if (charin == 0) return 32;
  else if (charin >= 1 && charin <= 26) return charin + 64;  //Scan uper case alphabet
  else if (charin >= 27 && charin <= 52) return charin + 70; //Scan lower case alphabet
  else if (charin >= 53 && charin <= 62) return charin - 5;  //Scan number
  else if (charin >= 63 && charin <= 77) return charin - 30; //Scan special character from space
  else if (charin >= 78 && charin <= 84) return charin - 20; //Scan special character :
  else if (charin >= 85 && charin <= 90) return charin + 6;  //Scan special character from [
  else if (charin >= 91 && charin <= 94) return charin + 32; //Scan special character from {
}

#endif //#ifndef NOUI
