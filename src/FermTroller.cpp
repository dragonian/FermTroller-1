/*  
  Copyright (C) 2009, 2010 Matt Reba, Jeremiah Dillingham

    This file is part of FermTroller.

    FermTroller is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FermTroller is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FermTroller.  If not, see <http://www.gnu.org/licenses/>.


FermTroller - Open Source Brewing Computer
Software Lead: Matt Reba (matt_AT_brewtroller_DOT_com)
Hardware Lead: Jeremiah Dillingham (jeremiah_AT_brewtroller_DOT_com)

Documentation, Forums and more information available at http://www.brewtroller.com
*/

/*
Compiled on Arduino-0022 (http://arduino.cc/en/Main/Software)
  With Sanguino Software "Sanguino-0018r2_1_4.zip" (http://code.google.com/p/sanguino/downloads/list)

  Using the following libraries:
    PID  v0.6 (Beta 6) (http://www.arduino.cc/playground/Code/PIDLibrary)
    OneWire 2.0 (http://www.pjrc.com/teensy/arduino_libraries/OneWire.zip)
    Encoder by CodeRage ()
    FastPin and modified LiquidCrystal with FastPin by CodeRage (http://www.brewtroller.com/forum/showthread.php?t=626)
*/



//*****************************************************************************************************************************
// BEGIN CODE
//*****************************************************************************************************************************
#include <avr/pgmspace.h>
#include <PID_Beta6.h>
#include <pin.h>
#include <menu.h>

#include "Config.h"
#include "Enum.h"
#include "HWProfile.h"
#include "PVOut.h"
#include "UI_LCD.h"

#include "FermTroller.h"
#include "FermCore.h"
#include "EEPROM.h"
#include "Outputs.h"
#include "Com.h"
#include "UI.h" 


const void(* softReset) (void) = 0;


//**********************************************************************************
// Compile Time Logic
//**********************************************************************************
#ifdef USE_I2C
  #include <Wire.h>
#endif

//**********************************************************************************
// Globals
//**********************************************************************************

#ifdef DIGITAL_INPUTS
  pin digInPin[6];
#endif

#ifdef HEARTBEAT
  pin hbPin;
#endif

//8-byte Temperature Sensor Address for each zone
byte tSensor[NUM_ZONES][8];
int temp[NUM_ZONES];

//Create the appropriate 'LCD' object for the hardware configuration (4-Bit GPIO, I2C)
#if defined UI_LCD_4BIT
  #include <LiquidCrystalFP.h>
  
  #ifndef UI_DISPLAY_SETUP
    LCD4Bit LCD(LCD_RS_PIN, LCD_ENABLE_PIN, LCD_DATA4_PIN, LCD_DATA5_PIN, LCD_DATA6_PIN, LCD_DATA7_PIN);
  #else
    LCD4Bit LCD(LCD_RS_PIN, LCD_ENABLE_PIN, LCD_DATA4_PIN, LCD_DATA5_PIN, LCD_DATA6_PIN, LCD_DATA7_PIN, LCD_BRIGHT_PIN, LCD_CONTRAST_PIN);
  #endif
  
#elif defined UI_LCD_I2C
  LCDI2C LCD(UI_LCD_I2CADDR);
#endif


//Valve Variables
unsigned long vlvConfig[NUM_VLVCFGS], actHeats, actCools;
boolean buzzStatus;
byte alarmStatus[NUM_ZONES];
boolean manualControl[NUM_ZONES];
unsigned long coolTime[NUM_ZONES];
byte coolMinOn[NUM_ZONES], coolMinOff[NUM_ZONES]; //Minimum On/Off time for coolOutput in minutes
byte coolMaxOn[NUM_ZONES]; // Maximum time for coolOutput in 30 second increments

//Create the appropriate 'Valves' object for the hardware configuration (GPIO, MUX, MODBUS)
#if defined PVOUT_TYPE_GPIO
  #define PVOUT
  PVOutGPIO Valves(PVOUT_COUNT);

#elif defined PVOUT_TYPE_MUX
  #define PVOUT
  PVOutMUX Valves( 
    MUX_LATCH_PIN,
    MUX_DATA_PIN,
    MUX_CLOCK_PIN,
    MUX_ENABLE_PIN,
    MUX_ENABLE_LOGIC
  );
  
#elif defined PVOUT_TYPE_MODBUS
  #define PVOUT
  PVOutMODBUS Valves();

#endif

//Shared buffers
char buf[20];

//Output Globals
double setpoint[NUM_ZONES];
byte hysteresis[NUM_ZONES];
byte alarmThresh[NUM_ZONES];

//Full Cool -100, Idle 0, Full Heat 100
int zonePwr[NUM_ZONES];

//Log Globals
boolean logData = LOG_INITSTATUS;

const char BT[] PROGMEM = "FermTroller";
const char BTVER[] PROGMEM = "2.1+MaxCool";

//Log Strings
const char LOGCMD[] PROGMEM = "CMD";
const char LOGDEBUG[] PROGMEM = "DEBUG";
const char LOGSYS[] PROGMEM = "SYS";
const char LOGCFG[] PROGMEM = "CFG";
const char LOGDATA[] PROGMEM = "DATA";

//**********************************************************************************
// Setup
//**********************************************************************************

void setup() {
  #ifdef USE_I2C
    Wire.begin(BT_I2C_ADDR);
  #endif
  
  //Log initialization (Log.pde)
  comInit();

  pinInit();

#ifdef PVOUT
  #if defined PVOUT_TYPE_GPIO
    #if PVOUT_COUNT >= 1
      Valves.setup(0, VALVE1_PIN);
    #endif
    #if PVOUT_COUNT >= 2
      Valves.setup(1, VALVE2_PIN);
    #endif
    #if PVOUT_COUNT >= 3
      Valves.setup(2, VALVE3_PIN);
    #endif
    #if PVOUT_COUNT >= 4
      Valves.setup(3, VALVE4_PIN);
    #endif
    #if PVOUT_COUNT >= 5
      Valves.setup(4, VALVE5_PIN);
    #endif
    #if PVOUT_COUNT >= 6
      Valves.setup(5, VALVE6_PIN);
    #endif
    #if PVOUT_COUNT >= 7
      Valves.setup(6, VALVE7_PIN);
    #endif
    #if PVOUT_COUNT >= 8
      Valves.setup(7, VALVE8_PIN);
    #endif
    #if PVOUT_COUNT >= 9
      Valves.setup(8, VALVE9_PIN);
    #endif
    #if PVOUT_COUNT >= 10
      Valves.setup(9, VALVEA_PIN);
    #endif
    #if PVOUT_COUNT >= 11
      Valves.setup(10, VALVEB_PIN);
    #endif
    #if PVOUT_COUNT >= 12
      Valves.setup(11, VALVEC_PIN);
    #endif
    #if PVOUT_COUNT >= 13
      Valves.setup(12, VALVED_PIN);
    #endif
    #if PVOUT_COUNT >= 14
      Valves.setup(13, VALVEE_PIN);
    #endif
    #if PVOUT_COUNT >= 15
      Valves.setup(14, VALVEF_PIN);
    #endif
    #if PVOUT_COUNT >= 16
      Valves.setup(15, VALVEG_PIN);
    #endif
  #endif
  Valves.init();
#endif

  tempInit();
  
  //Check for cfgVersion variable and update EEPROM if necessary (EEPROM.pde)
  checkConfig();
  
  //Load global variable values stored in EEPROM (EEPROM.pde)
  loadSetup();
  
  //User Interface Initialization (UI.pde)
  //Moving this to last of setup() to allow time for I2CLCD to initialize
  #ifndef NOUI
    uiInit();
  #endif
  
  splashScreen();
  screenInit(0);
  unlockUI();
}


//**********************************************************************************
// Loop
//**********************************************************************************

void loop() {
  //User Interface Processing (UI.pde)
  #ifndef NOUI
    uiCore();
  #endif
  
  //Core FermTroller process code (FermCore.pde)
  fermCore();
}



int main(void)
{
    init();

    setup();

    for (;;) {
        loop();
        if (serialEventRun) serialEventRun();
    }

    return 0;
}
