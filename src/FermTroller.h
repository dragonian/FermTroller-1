#ifndef BREWTROLLER_H
#define BREWTROLLER_H

#define BUILDNUM 2

#include <Arduino.h>
#include <pin.h>
#include <PID_Beta6.h>

#include "HardwareProfile.h"
#include "Temp.h"
#include "PVOut.h"
#include "UI_LCD.h"

const extern void(* softReset) (void);

//**********************************************************************************
// Compile Time Logic
//**********************************************************************************

#ifndef NUM_ZONES
  #define NUM_ZONES PVOUT_COUNT
#endif

#define NUM_VLVCFGS NUM_ZONES * 2 + 1 //Per zone Heat and Cool + Global Alarm


#ifdef USEMETRIC
  #define SETPOINT_MULT 50
  #define SETPOINT_DIV 2
#else
#define SETPOINT_MULT 100
#define SETPOINT_DIV 1
#endif


#if COM_SERIAL0 == BTNIC || defined BTNIC_EMBEDDED
#define BTNIC_PROTOCOL
#endif

#if defined BTPD_SUPPORT || defined UI_LCD_I2C || defined TS_ONEWIRE_I2C || defined BTNIC_EMBEDDED || defined RGBIO8_ENABLE
#define USE_I2C
#endif

//**********************************************************************************
// Globals
//**********************************************************************************
//Heat Output Pin Array
extern pin heatPin[4], alarmPin;

#ifdef DIGITAL_INPUTS
extern pin digInPin[DIGIN_COUNT];
#endif

#ifdef HEARTBEAT
extern pin hbPin;
#endif

//8-byte Temperature Sensor Address x9 Sensors
extern byte tSensor[NUM_ZONES][8];
extern int temp[NUM_ZONES];

//Create the appropriate 'LCD' object for the hardware configuration (4-Bit GPIO, I2C)
#if defined UI_LCD_4BIT
  #include <LiquidCrystalFP.h>

  #ifndef UI_DISPLAY_SETUP
    extern LCD4Bit LCD;
  #else
    extern LCD4Bit LCD;
  #endif

#elif defined UI_LCD_I2C
extern LCDI2C LCD;
#endif

//Valve Variables
extern unsigned long vlvConfig[NUM_VLVCFGS], actHeats, actCools;
extern boolean buzzStatus;
extern byte alarmStatus[NUM_ZONES];
extern boolean manualControl[NUM_ZONES];
extern unsigned long coolTime[NUM_ZONES];
extern byte coolMinOn[NUM_ZONES], coolMinOff[NUM_ZONES]; //Minimum On/Off time for coolOutput in minutes
extern byte coolMaxOn[NUM_ZONES]; // Maximum time for coolOutput


#if defined PVOUT_TYPE_GPIO
  #define PVOUT
  extern PVOutGPIO Valves;

#elif defined PVOUT_TYPE_MUX
  #define PVOUT
  extern PVOutMUX Valves;
  
#elif defined PVOUT_TYPE_MODBUS
  #define PVOUT
  extern PVOutMODBUS Valves;

#endif


//Shared buffers
extern char buf[20];

//Output Globals
extern double setpoint[NUM_ZONES];
extern byte hysteresis[NUM_ZONES];
extern byte alarmThresh[NUM_ZONES];

//Full Cool -100, Idle 0, Full Heat 100
extern int zonePwr[NUM_ZONES];

//Log Globals
extern boolean logData;

extern const char BT[];
extern const char BTVER[];

//Log Strings
extern const char LOGCMD[];
extern const char LOGDEBUG[];
extern const char LOGSYS[];
extern const char LOGCFG[];
extern const char LOGDATA[];

#endif
