#ifndef BT_CONFIGURATION
#define BT_CONFIGURATION

//*****************************************************************************************************************************
// USER COMPILE OPTIONS
//*****************************************************************************************************************************

//**********************************************************************************
// Number of Zones (NUM_ZONES)
//**********************************************************************************
// The number of temperature zones to control. You may wish to add a zone for 
// ambient temperature outside your fermentation/conditioning zones for monitoring
// purposes. If this value is not specified NUM_ZONES defaults to the number of
// outputs on the system as defined by PVOUT_COUNT in HWProfile.h
//
// Theoretical Maximum: 32 Zones
//
// #define NUM_ZONES 4
//**********************************************************************************

//**********************************************************************************
// UNIT (Metric/US)
//**********************************************************************************
// By default BrewTroller will use US Units
// Uncomment USEMETRIC below to use metric instead
//
//#define USEMETRIC
//**********************************************************************************

//**********************************************************************************
// Setpoint Limits
//**********************************************************************************
// SETPOINT_MIN: Minimum allowed setpoint in hundreths of a degree
// SETPOINT_MAX: Maximum allowed setpoint in hundreths of a degree

#ifdef USEMETRIC
  //Metric Limits
  #define SETPOINT_MIN -500 //-5C
  #define SETPOINT_MAX 4000 //40C
#else
  //Imperial Limits
  #define SETPOINT_MIN 2000 //20F
  #define SETPOINT_MAX 10000 //100F
#endif  
//**********************************************************************************

//**********************************************************************************
// Alarm Boot Up Delay
//**********************************************************************************
// ALARM_BOOTUP_DELAY: Ignore alarms for a period of time to allow initial
// temperature conversion to complete
//
#define ALARM_BOOTUP_DELAY 2000
//**********************************************************************************

//**********************************************************************************
// Buzzer modulation parameters
//**********************************************************************************
// These parameters allow the alarm sound to be modulated. 
// The modulation occurs when the BUZZER_CYCLE_TIME value is larger than the BUZZER_ON_TIME
// When the BUZZER_CYCLE_TIME is zero there is no modulation so the buzzer will buzz  
// a steady sound
//
//#define BUZZER_CYCLE_TIME 1200 //the value is in milliseconds for the ON and OFF buzzer cycle
//#define BUZZER_ON_TIME 500     //the duration in milliseconds where the alarm will stay on
//**********************************************************************************


//**********************************************************************************
// Serial0 Communication Options
//**********************************************************************************
// COM_SERIAL0: Specifies the communication type being used (Pick One):
//  ASCII  = Original BrewTroller serial command protocol used with BTRemote and BTLog
//  BTNIC  = BTnic (Lighterweight implementation of ASCII protocol using single-byte
//           commands. This protocol is used with BTnic Modules and software for
//           network connectivity.
//  BINARY = Binary Messages
//**********************************************************************************

#define COM_SERIAL0  ASCII
//#define COM_SERIAL0  BTNIC
//#define COM_SERIAL0 BINARY

// BAUD_RATE: The baud rate for the Serial0 connection. Previous to BrewTroller 2.0
// Build 419 this was hard coded to 9600. Starting with Build 419 the default rate
// was increased to 115200 but can be manually set using this compile option.
#define SERIAL0_BAUDRATE 115200


// ** ASCII Protocol Options:
//
// COMSCHEMA: Specifies the schema for a particular type
//  ASCII Messages
//      0 - Original BT 2.0 Messages
//      1 - BT 2.1 Enhanced ASCII
//       Steam, Calc. Vol & Temp, BoilPower, Grain Temp, Delay Start, MLT Heat Source
#define COMSCHEMA 0
//
// LOG_INTERVAL: Specifies how often data is logged via serial in milliseconds. If
// real time display of data is being used a smaller interval is best (1000 ms). A
// larger interval can be used for logging applications to reduce log file size 
// (5000 ms).
#define LOG_INTERVAL 2000
//
// LOG_INITSTATUS: Sets whether logging is enabled on bootup. Log status can be
// toggled using the SET_LOGSTATUS command.
#define LOG_INITSTATUS 0

//**********************************************************************************
// BTnic Embedded Module
//**********************************************************************************
#define BTNIC_EMBEDDED


//**********************************************************************************
// BrewTroller PID Display (BTPD)
//**********************************************************************************
// BTPD is an external LED display developed by BrewTroller forum member vonnieda. 
// It is a 2 line, 4 digit (8 digits total) LED display with one line red and one
// line green. The digits are about a half inch high and can easily be seen across
// the room. The display connects to the BrewTroller via the I2C header and can be
// daisy chained to use as many as you like, theoretically up to 127 but in practice
// probably 10 or so.

// BTPD_SUPPORT: Enables use of BrewTroller PID Display devices on I2C bus
//#define BTPD_SUPPORT

// BTPD_INTERVAL: Specifies how often BTPD devices are updated in milliseconds
#define BTPD_INTERVAL 1000
//**********************************************************************************


#endif

