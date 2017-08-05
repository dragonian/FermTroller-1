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

  Update 9/22/2010 to support enhanced functions and mutiple schemas.
  
*/
#include "Config.h"
#include "Enum.h"
#include <avr/eeprom.h>
#include <EEPROM.h>
#include "FermTroller.h"  
#include "EEPROM.h"
#include "Outputs.h"

#ifdef COM_SERIAL0
#if COM_SERIAL0 == ASCII

void sendOK();
void rejectParam();
void clearMsg();
void logVersion();
void clearMsg();
void logOSet(byte);
void logTSensor(byte);
void logVlvConfig(byte);
void logVersion();
void logASCIIVersion();
void getLog();
void logAlarm(byte);



//**********************************************************************************
//Code for Schemas 0 & 1
//**********************************************************************************
unsigned long lastLog;
boolean msgQueued;
byte logCount, msgField;
char msg[CMD_MSG_FIELDS][CMD_FIELD_CHARS];

void logStart_P (const char *sType) {
 Serial.print(millis(),DEC);
 Serial.print("\t");
 while (pgm_read_byte(sType) != 0) Serial.print(pgm_read_byte(sType++)); 
 Serial.print("\t");
}

void logEnd () {
 Serial.println();
}

void logField (char sText[]) {
  Serial.print(sText);
  Serial.print("\t");
}

void logFieldUL (unsigned long value) {
  Serial.print(value, DEC);
  Serial.print("\t");
}

void logFieldL (long value) {
  Serial.print(value, DEC);
  Serial.print("\t");
}

void logField_P (const char *sText) {
  while (pgm_read_byte(sText) != 0) Serial.print(pgm_read_byte(sText++));
  Serial.print("\t");
}

//This is (and should only) be used internally in COMSCHEMA 0/1
void logString_P (const char *sType, const char *sText) {
 logStart_P(sType);
 logField_P(sText);
 logEnd();
}

boolean chkMsg() {
  if (!msgQueued) {
    while (Serial.available()) {
      byte byteIn = Serial.read();
      if (byteIn == '\r') { 
        msgQueued = 1;
        //Configuration Class (CFG) Commands
        if(strcasecmp(msg[0], "GET_OSET") == 0) {
          byte val = atoi(msg[1]);
          if (msgField == 1 && val < NUM_ZONES * 2) {
            logOSet(val);
            clearMsg();
          } else rejectParam();
        } else if (strcasecmp(msg[0], "GET_TS") == 0) {
          byte val = atoi(msg[1]);
          if (msgField == 1 && val < NUM_ZONES) {
            logTSensor(val);
            clearMsg();
          } else rejectParam();
        } else if(strcasecmp(msg[0], "GET_VLVCFG") == 0) {
          byte profile = atoi(msg[1]);
          if (msgField == 1 && profile < NUM_VLVCFGS) {
            logVlvConfig(profile);
            clearMsg();
          } else rejectParam();
        } else if(strcasecmp(msg[0], "INIT_EEPROM") == 0) {
          clearMsg();
          initEEPROM();
        } else if(strcasecmp(msg[0], "SCAN_TS") == 0) {
          clearMsg();
          byte tsAddr[8];
          getDSAddr(tsAddr);
          logStart_P(LOGCFG);
          logField_P(PSTR("TS_SCAN")); 
          for (byte i=0; i<8; i++) logFieldL(tsAddr[i]);
          logEnd();
        } else if(strcasecmp(msg[0], "SET_OSET") == 0) {
          byte val = atoi(msg[1]);
          if (msgField == 2 && val < NUM_ZONES * 2) {
            setHysteresis(val, (byte)atoi(msg[2]));
            clearMsg();
            logOSet(val);
          } else rejectParam();
        } else if(strcasecmp(msg[0], "SET_TS") == 0) {
          byte val = atoi(msg[1]);
          if (msgField == 9 && val < NUM_ZONES) {
            byte addr[8];
            for (byte i=0; i<8; i++) addr[i] = (byte)atoi(msg[i+2]);
            setTSAddr(val, addr);
            clearMsg();
            logTSensor(val);
          } else rejectParam();
        } else if(strcasecmp(msg[0], "SET_VLVCFG") == 0) {
          byte profile = atoi(msg[1]);
          if (msgField == 2 && profile < NUM_VLVCFGS) {
            setValveCfg(profile, strtoul(msg[2], NULL, 10));
            clearMsg();
            logVlvConfig(profile);
          } else rejectParam();
        } else if(strcasecmp(msg[0], "SET_ALARM") == 0) {
          byte zone = atoi(msg[1]);
          if (msgField == 2) {
            setAlarmStatus(zone, atoi(msg[2]));
            sendOK();
          } else rejectParam();
        } else if(strcasecmp(msg[0], "SET_SETPOINT") == 0) {
          byte vessel = atoi(msg[1]);
          if (msgField == 2 && vessel < NUM_ZONES) {
            setSetpoint(vessel, (byte)atoi(msg[2]));
            sendOK();
          } else rejectParam();
        } else if(strcasecmp(msg[0], "SET_VLV") == 0) {
          rejectParam(); //Command no longer supported
        } else if(strcasecmp(msg[0], "SET_VLVPRF") == 0) {
          rejectParam(); //Command no longer supported

        //System Class (SYS) Commands
        } else if(strcasecmp(msg[0], "RESET") == 0) {
          byte level = atoi(msg[1]);
          if (msgField == 1 && level >= 0 && level <= 1) {
            clearMsg();
            if (level == 0) {
              resetOutputs();
            } else if (level == 1) {
              logStart_P(LOGSYS);
              logField_P(PSTR("SOFT_RESET"));
              logEnd();
              softReset();
            }
          } else rejectParam();
        } else if(strcasecmp(msg[0], "SET_LOGSTATUS") == 0) {
          if (msgField == 1) {
            logData = (boolean)atoi(msg[1]);
            sendOK();
          } else rejectParam();
        } else if(strcasecmp(msg[0], "GET_VER") == 0) {
          logVersion();
          clearMsg();        

// Schema 1 Functions
//
#if COMSCHEMA > 0
        } else if(strcasecmp(msg[0], "GET_LOG") == 0) {
          if (msgField == 1) {
            logCount = atoi(msg[1]);
            getLog();
            clearMsg();
          } else rejectParam();
        } else if(strcasecmp(msg[0], "GET_ALARM") == 0) {
          logAlarm();
          clearMsg();        
        } else if(strcasecmp(msg[0], "GET_LOGSTATUS") == 0) {
          logLogStatus();
          clearMsg();
        } else if(strcasecmp(msg[0], "GET_EEPROM") == 0) {
        if (msgField == 2) {
          int address = atoi(msg[1]);
          int length = atoi(msg[2]);
          if (address >= 0 && address < 2048 &&
              length > 0 && length <= 64)
          {
            getEEPROM(address, length);
            clearMsg();
          }
        } else rejectParam();
      } else if(strcasecmp(msg[0], "SET_EEPROM") == 0) {
          setEEPROM();
          clearMsg();
#endif

        //End of Commands
        }
        break;

      } else if (byteIn == '\t') {
        if (msgField < CMD_MSG_FIELDS) {
          msgField++;
        } else {
          logString_P(LOGCMD, PSTR("MSG_OVERFLOW"));
          clearMsg();
        }
      } else {
        byte charCount = strlen(msg[msgField]);
        if (charCount < CMD_FIELD_CHARS - 1) { 
          msg[msgField][charCount] = byteIn; 
          msg[msgField][charCount + 1] = '\0';
        } else {
          logString_P(LOGCMD, PSTR("FIELD_OVERFLOW"));
          clearMsg();
        }
      }
    }
  }
  if (msgQueued) return 1; else return 0;
}

void sendOK() {
  #if COMSCHEMA > 0
    logStart_P(LOGCMD);
    logField_P(PSTR("OK"));
    logEnd();
  #endif
  clearMsg();
}

void clearMsg() {
  msgQueued = 0;
  msgField = 0;
  for (byte i = 0; i < CMD_MSG_FIELDS; i++) msg[i][0] = '\0';
}

void rejectMsg() {
  logStart_P(LOGCMD);
  logField_P(PSTR("UNKNOWN_CMD"));
  for (byte i = 0; i < msgField; i++) logField(msg[i]);
  logEnd();
  clearMsg();
}

void rejectParam() {
  logStart_P(LOGCMD);
  logField_P(PSTR("BAD_PARAM"));
  for (byte i = 0; i <= msgField; i++) logField(msg[i]);
  logEnd();
  clearMsg();
}

void logVersion() { logASCIIVersion(); }

void updateS0ASCII() {
  // All data is logged every 2s. A total of 28 records are sent. The 
  //  records are sent on consecutive scans.
  if (logData) {
    unsigned long now = millis();
    if (now < lastLog) lastLog = 0; //Timer overflow has occurred
    if (now - lastLog > LOG_INTERVAL) {
      getLog();
      if (logCount == NUM_ZONES) 
      {
        //Logic below times start of log to start of log. Interval is reset if exceeds two intervals.
        if (now - lastLog > LOG_INTERVAL * 2) 
          lastLog = now; 
        else 
          lastLog += LOG_INTERVAL;
      }
      logCount++;
      if (logCount > NUM_ZONES)
        logCount = 0;  
    }    

  }
  // if logData is false && chkMsg() is false - should we pass the LOGCFG constant into rejectMsg?
  if (chkMsg()) rejectMsg();   // old value: LOGGLB
}

void getLog() {
  if (logCount < NUM_ZONES) {
    byte i = logCount;
    logStart_P(LOGDATA);
    logField_P(PSTR("TEMP"));
    logFieldL(i);
    logFieldL(temp[i]);
    #if COMSCHEMA == 0
      #ifdef USEMETRIC
        logFieldL(0);
      #else
        logFieldL(1);
      #endif
    #endif
    logEnd();
    logStart_P(LOGDATA);
    logField_P(PSTR("ZONEPWR"));
    logFieldL(i);
    logFieldL(zonePwr[i]);
    logEnd();
    logStart_P(LOGDATA);
    logField_P(PSTR("SETPOINT"));
    logFieldL(i);
    unsigned long tempval = setpoint[i];
    tempval = tempval / SETPOINT_MULT;
    logFieldL(tempval);
    #if COMSCHEMA == 0
      #ifdef USEMETRIC
        logFieldL(0);
      #else
        logFieldL(1);
      #endif
    #endif
    logEnd();
    logAlarm(i);

  } else if (logCount == NUM_ZONES) {
    logStart_P(LOGDATA);
    logField_P(PSTR("VLVPRF"));
    logFieldUL(actHeats);
    logFieldUL(actCools);
    logEnd();
  }
}

void logOSet(byte vessel) {
  logStart_P(LOGCFG);
  logField_P(PSTR("OUTPUT_SET"));
  logFieldL(vessel);
  logFieldL(hysteresis[vessel]);
  logEnd();
}

void logTSensor(byte sensor) {
  logStart_P(LOGCFG);
  logField_P(PSTR("TS_ADDR"));
  logFieldL(sensor);
  for (byte i=0; i<8; i++) logFieldL(tSensor[sensor][i]);
  logEnd();
}

void logAlarm(byte zone) {
  logStart_P(LOGDATA);
  logField_P(PSTR("ALARM"));
  logFieldL(alarmStatus[zone]);
  logEnd();
}

void logVlvConfig (byte profile) {
  logStart_P(LOGCFG);
  logField_P(PSTR("VLV_CONFIG"));
  logFieldL(profile);
  logFieldUL(vlvConfig[profile]);  
  logEnd();
}

void logLogStatus() {
  logStart_P(LOGSYS);
  logField_P(PSTR("LOG_STATUS"));
  logFieldL(logData);
  logEnd();
}

// Schema 1 Functions
//

#if COMSCHEMA > 0

// read values from EEPROM
//
void getEEPROM(int address, int dataLen) 
{
  byte checksum = 0;

  logStart_P(LOGCFG);
  logField_P(PSTR("EEPROM"));
  
  // address & data length
  checksum += printHex8(byte(address>>8));
  checksum += printHex8(byte(address));
  Serial.print('\t');
  checksum += printHex8(dataLen);
  
  // EEPROM data
  for (int i=0; i < dataLen; i++)
  {
    if (i % 8 == 0) 
      Serial.print('\t');
    checksum += printHex8(EEPROM.read(address++));
  }

  // checksum
  Serial.print('\t');
  checksum = (~checksum) + 1;
  printHex8(checksum);

  Serial.print('\t');
  logEnd();
}

//  write values to EEPROM
//
//
bool setEEPROM() {
  byte* btBuf = (byte*)&msg;  // msg buffer will be used to store data to write in EEPROM
  byte msgIndex = 2;          // msg[3] is first chunk of data
  byte checksum = 0;

  // get address and data length
  //
  int address = hex2Int(msg[1], 0, 4);
  checksum += byte(address>>8);
  checksum += byte(address);
  byte dataLen = hex2Int(msg[2], 0, 2);
  checksum += dataLen;

  // data loop
  //
  byte i;
  for (i=0; i < dataLen; i++)
  {
    byte chIndex = 2 * (i & 0x07); // character index (i % 8)
    // check for start of next msg[]
    if (chIndex == 0) {
      msgIndex++;
    }
    // get data and put in buffer
    btBuf[i] = (byte)hex2Int(msg[msgIndex], chIndex, 2);
    checksum += btBuf[i]; 
  }

  // get checksum
  //
  msgIndex++;
  btBuf[i] = (byte)hex2Int(msg[msgIndex], 0, 2);
  checksum += btBuf[i];

  // checksum check (should be 0 if valid)
  //
  if (checksum != 0)
  {
    returnChecksumError();
    return false;
  }

  // write values to EEPROM
  //
  for (byte i=0; i < dataLen; i++)
  {
    EEPROM.write(address+i, btBuf[i]);
  }

  getEEPROM(address, dataLen);
  //testSetEEPROM(address, dataLen);

  return true;
}

void returnChecksumError() {
  logStart_P(LOGCMD);
  logField_P(PSTR("CHECKSUM_ERROR"));
  logEnd();
  clearMsg();
}

int hex2Int(char chBuf[], byte offset, byte length) {

  int iVal=0;
  for (byte i=0; i<length; i++) 
  {
    iVal <<= 4;
    char hexChar = chBuf[offset+i];
    
    // make upper case
    if (hexChar >= 0x60) 
      hexChar -= 0x20;  
    
    // convert to value
    if (hexChar >= '0' && hexChar <= '9')
      iVal += hexChar - '0';
    else if (hexChar >= 'A' && hexChar <= 'F')
      iVal += hexChar - 0x41 + 10;
    else
      return -1;
  }
  return iVal;
}

byte printHex8(byte btVal) // prints 8-bit data in hex with leading zeroes
{
  if (btVal<0x10) 
    Serial.print("0");
  Serial.print(btVal,HEX);
  return btVal;
}
#endif  //COMSCHEMA > 0

#endif  // COM_SERIAL0 == ASCII
#endif
