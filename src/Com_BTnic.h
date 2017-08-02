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

#ifdef BTNIC_PROTOCOL
  #include "Config.h"
  #include "Enum.h"

/********************************************************************************************************************
 * BTnic Class
 ********************************************************************************************************************/
  // Using Values 65-90 & 97-122 for command codes to make terminal input easier 
  // but any value between 0-255 can be used with the following exceptions
  // Command Field Char		9	(Tab)
  // Command Term Char		10	(Line Feed)
  // Command Term Char		13	(Carriage Return)

  //Command codes for special responses
  #define CMD_REJECT        33	//!
  #define CMD_REJECT_PARAM  35	//#
  #define CMD_REJECT_INDEX  36	//$
  #define CMD_REJECT_CRC     42	//*

  #define CMD_GET_OSET		68 	//D
  #define CMD_GET_TS		70 	//F
  #define CMD_GET_VER		71 	//G
  #define CMD_INIT_EEPROM	73 	//I
  #define CMD_SCAN_TS		74 	//J
  #define CMD_SET_OSET		78 	//N
  #define CMD_SET_TS		80 	//P
  #define CMD_SET_VLVCFG	81 	//Q
  #define CMD_SET_ALARM		86 	//V
  #define CMD_SET_SETPOINT	88 	//X
  #define CMD_SET_VLV		97 	//a (No longer Supported)
  #define CMD_SET_VLVPRF	98 	//b (Not supported)
  #define CMD_RESET		99 	//c
  #define CMD_GET_VLVCFG	100 	//d
  #define CMD_GET_ALARM		101 	//e
  #define CMD_TEMP		113 	//q
  #define CMD_ZONEPWR    	115 	//s
  #define CMD_SETPOINT		116 	//t
  #define CMD_VLVBITS		118 	//v
  #define CMD_VLVPRF		119 	//w
  
  #define BTNIC_STATE_RX 0
  #define BTNIC_STATE_EXE 1
  #define BTNIC_STATE_TX 2
  
  #define BTNIC_BUF_LEN 256

  #define CMDCODE_MIN 68
  #define CMDCODE_MAX 119
  #define NO_CMDINDEX -1
  
  static byte CMD_PARAM_COUNTS[] PROGMEM = 
  {
    0,	//CMD_GET_OSET
    0,	  //Not Used
    0,	//CMD_GET_TS
    0,	//CMD_GET_VER
    0,	  //Not Used
    1,	//CMD_INIT_EEPROM
    0,	//CMD_SCAN_TS
    0,	  //Not Used
    0,	  //Not Used
    0,	  //Not Used
    1,	//CMD_SET_OSET
    0,	  //Not Used
    8,	//CMD_SET_TS
    1,	  //Not Used
    0,	  //Not Used
    0,	  //Not Used
    0,	  //Not Used
    0,	  //Not Used
    1,	//CMD_SET_ALARM
    0,	  //Not Used
    1,	//CMD_SET_SETPOINT
    0,	  //Not Used
    0,	  //Not Used
    0,    //Not Used
    0,    //Not Used
    0,    //Not Used
    0,    //Not Used
    0,    //Not Used
    0,    //Not Used
    0,	//CMD_SET_VLV (No Longer Used)
    2,	//CMD_SET_VLVPRF
    0,	//CMD_RESET
    0,	//CMD_GET_VLVCFG
    0,	//CMD_GET_ALARM
    0,	  //Not Used
    0,	  //Not Used
    0,	  //Not Used
    0,	  //Not Used
    0,	  //Not Used
    0,	  //Not Used
    0,	  //Not Used
    0,	  //Not Used
    0,	  //Not Used
    0,	  //Not Used
    0,	  //Not Used
    0,	//CMD_TEMP
    0,	  //Not Used
    0,	//CMD_ZONEPWR
    0,	//CMD_SETPOINT
    0,	  //Not Used
    0,	//CMD_VLVBITS
    0 	//CMD_VLVPRF
  };

  static byte CMD_INDEX_MAXVALUE[] PROGMEM = 
  {
    NUM_ZONES * 2 - 1,	//CMD_GET_OSET
    0,   		  //Not Used
    NUM_ZONES - 1, 	//CMD_GET_TS
    0, 			//CMD_GET_VER
    0,   		  //Not Used
    0, 			//CMD_INIT_EEPROM
    0, 			//CMD_SCAN_TS
    0, 			  //Not Used
    0,    		  //Not Used
    0, 			  //Not Used
    NUM_ZONES * 2 - 1,  //CMD_SET_OSET
    0, 	                  //Not Used
    NUM_ZONES - 1, 	//CMD_SET_TS
    0,           	  //Not Used
    0, 		          //Not Used
    0,                    //Not Used
    0,                    //Not Used
    0,                    //Not Used
    NUM_ZONES - 1, 	//CMD_SET_ALARM
    0,                    //Not Used
    0,                    //Not Used
    0,                    //Not Used
    0,                    //Not Used
    0,  		  //Not Used
    0,  		  //Not Used
    0,   		  //Not Used
    0,  		  //Not Used
    0,  		  //Not Used
    0,  		  //Not Used
    0, 			  //CMD_SET_VLV (No Longer Used)
    0, 			//CMD_SET_VLVPRF
    1, 			//CMD_RESET
    NUM_VLVCFGS - 1, 	//CMD_GET_VLVCFG
    NUM_ZONES - 1, 	//CMD_GET_ALARM
    0, 			  //Not Used
    0, 			  //Not Used
    0, 			  //Not Used
    0, 			  //Not Used
    0, 			  //Not Used
    0, 			  //Not Used
    0, 	                  //Not Used
    0, 	                  //Not Used
    0, 			  //Not Used
    0, 	                  //Not Used
    0, 		          //Not Used
    NUM_ZONES - 1, 		//CMD_TEMP
    0, 			//Not Used
    NUM_ZONES - 1, 	//CMD_ZONEPWR
    NUM_ZONES - 1, 	//CMD_SETPOINT
    0, 			  //Not Used
    0, 			//CMD_VLVBITS
    0  			//CMD_VLVPRF
  };
  
class BTnic
{
public:
  BTnic(void); /* Constructor */
  void rx(char); /* Receive a byte into buffer and returns true of tx ready*/
  char tx(void); /* Return a byte from buffer or '/0' if end of buffer */
  void reset(void); /* Sets buf len to 0 */
  byte getState(void); /* Return current state RX / EXE / TX */
  void eventHandler(byte, int);
  
private:
  void execCmd(void);
  void rejectCmd(byte);
  void chkBuf(void);
  void logFieldCmd(byte, int);
  void logField(char*);
  void logField_P(const char*);
  void logFieldI(unsigned long);
  void logEnd(void);
  int getCmdIndex(void);
  byte getCmdParamCount(void);
  char* getCmdParam(byte, char*, byte);
  unsigned long getCmdParamNum(byte);
  
  byte _state; /* Current state: RX/EXE_R/EXE_W/TX */
  unsigned int _bufLen; /* Length of data in buffer */
  char _bufData[BTNIC_BUF_LEN]; /* Buffer */
  unsigned int _bufCur; /* Cursor position in buffer for tx */
};

BTnic::BTnic(void) {
  reset();
}

void BTnic::rx(char byteIn) {
  if (byteIn == 0x0D) execCmd();
  else {
    _bufData[_bufLen++] = byteIn;
    if (_bufLen == BTNIC_BUF_LEN) execCmd();
  }
}

char BTnic::tx(void) {
  if (_bufCur < _bufLen) return _bufData[_bufCur++];
  else return '\0';
}

void BTnic::reset(void) {
  _bufLen = 0;
  _bufCur = 0;
  _state = BTNIC_STATE_RX;
}

byte BTnic::getState(void) { 
  if (_state == BTNIC_STATE_TX && _bufCur == _bufLen) reset();
  return _state;
}

void BTnic::eventHandler(byte eventID, int eventParam) {
  //Not Implemented
}


  //Check and process command. Return error code (0 if OK)
void BTnic::execCmd(void) {
  _state = BTNIC_STATE_EXE;

  // log ASCII version "GET_VER"
  if (strcasecmp(getCmdParam(0, buf, 20), "GET_VER") == 0) {
    logASCIIVersion();
    return reset();
  }

  if(!_bufLen || _bufData[0] < CMDCODE_MIN || _bufData[0] > CMDCODE_MAX) return rejectCmd(CMD_REJECT);
  if(getCmdParamCount() != pgm_read_byte(CMD_PARAM_COUNTS + _bufData[0] - CMDCODE_MIN)) return rejectCmd(CMD_REJECT_PARAM);
  int cmdIndex = getCmdIndex();
  if(cmdIndex == NO_CMDINDEX) return rejectCmd(CMD_REJECT_INDEX);

  switch (_bufData[0]) {
    case CMD_SET_OSET:  //N
      setHysteresis(cmdIndex, getCmdParamNum(1));
    case CMD_GET_OSET:  //D
      logFieldCmd(CMD_GET_OSET, cmdIndex);
      logFieldI(hysteresis[cmdIndex]);
      break;

    case CMD_SET_TS:  //P
      {
        byte addr[8];
        for (byte i=0; i<8; i++) addr[i] = (byte)getCmdParamNum(i+1);
        setTSAddr(cmdIndex, addr);
      }
    case CMD_GET_TS:  //F
      logFieldCmd(CMD_GET_TS, cmdIndex);
      for (byte i=0; i<8; i++) logFieldI(tSensor[cmdIndex][i]);
      break;
      
      
    case CMD_GET_VER:  //G
      logFieldCmd(CMD_GET_VER, NO_CMDINDEX);
      logField_P(BTVER);
      logFieldI(BUILD);
      logFieldI(BTNIC);
      logFieldI(COMSCHEMA);
      #ifdef USEMETRIC
        logFieldI(0);
      #else
        logFieldI(1);
      #endif
      break;


    case CMD_INIT_EEPROM:  //I
      if (getCmdParamNum(1) != 210) return rejectCmd(CMD_REJECT_PARAM);
      logFieldCmd(CMD_INIT_EEPROM, NO_CMDINDEX);
      initEEPROM();
      break;
      

    case CMD_SCAN_TS:  //J
      {
        logFieldCmd(CMD_SCAN_TS, NO_CMDINDEX);
        byte tsAddr[8];
        getDSAddr(tsAddr);
        for (byte i=0; i<8; i++) logFieldI(tsAddr[i]);
      }
      break;
      
      
    case CMD_SET_VLVCFG:  //Q
      setValveCfg(cmdIndex, getCmdParamNum(1));
    case CMD_GET_VLVCFG:  //d
      logFieldCmd(CMD_GET_VLVCFG, cmdIndex);
      logFieldI(vlvConfig[cmdIndex]);  
      break; 

/*
    case CMD_SET_ALARM:  //V
      setAlarm(getCmdParamNum(1));
    case CMD_GET_ALARM:  //e
      logFieldCmd(CMD_GET_ALARM, NO_CMDINDEX);
      logFieldI(alarmStatus);
      break;
*/
      
    case CMD_SET_SETPOINT:  //X
      setSetpoint(cmdIndex, getCmdParamNum(1));
    case CMD_SETPOINT:  //t
      logFieldCmd(CMD_SETPOINT, cmdIndex);
      logFieldI(setpoint[cmdIndex] / SETPOINT_MULT);
      break;
      

    case CMD_VLVPRF:  //w
      logFieldCmd(CMD_VLVPRF, NO_CMDINDEX);
      logFieldI(actHeats);
      logFieldI(actCools);
      break;
      
      
    case CMD_RESET:  //c
      //Reboot (1) or just Reset Outputs?
      if (cmdIndex == 1) softReset();
      else {
        logFieldCmd(CMD_RESET, cmdIndex);
        resetOutputs();
      }
      break;
      

    case CMD_TEMP:  //q
      logFieldCmd(CMD_TEMP, cmdIndex);
      logFieldI(temp[cmdIndex]);
      break;
      
      
    case CMD_ZONEPWR:  //s
      logFieldCmd(CMD_ZONEPWR, cmdIndex);
      logFieldI(zonePwr[cmdIndex]);
      break;
      

    case CMD_VLVBITS:  //v
      logFieldCmd(CMD_VLVBITS, NO_CMDINDEX);
      logFieldI(computeValveBits());
      break;
      
      
    default: 
      return rejectCmd(CMD_REJECT); //Reject Command Code (CMD_REJECT);
  }
  logEnd();
}
  
void BTnic::rejectCmd(byte rejectCode) {
  //Unknown Command: Shift buffer two chars and insert rejectCode and field delimeter
  if (_bufLen) {
    _bufLen = min(_bufLen + 2, BTNIC_BUF_LEN);
    _bufCur = _bufLen - 1;
    while (_bufCur > 1) _bufData[_bufCur] = _bufData[(_bufCur--) - 2];
    _bufData[1] = 0x09;
  } else _bufLen = 1;
  _bufData[0] = rejectCode;
  logEnd();  
}

void BTnic::logFieldCmd(byte cmdCode, int cmdIndex) {
  char tmpbuf[8];
  tmpbuf[0] = cmdCode;
  tmpbuf[1] = '\0';
  if (cmdIndex != -1) {
    char indexBuf[7];
    itoa(cmdIndex, indexBuf, 10);
    strcat(tmpbuf, indexBuf);
  }
  _bufLen = 0;
  logField(tmpbuf);
}

void BTnic::logField(char *string) {
  if (_bufLen) _bufData[_bufLen++] = 0x09;  //Tab Char if not being used for cmd field
  while (*string != '\0') _bufData[_bufLen++] = *(string++);
}
  
void BTnic::logField_P(const char *string) {
  _bufData[_bufLen++] = 0x09;  //Tab Char
  while (pgm_read_byte(string) != 0) _bufData[_bufLen++] = pgm_read_byte(string++);
}

void BTnic::logFieldI(unsigned long param) {
  char tmpbuf[11];
  ultoa(param, tmpbuf, 10);
  logField(tmpbuf);
}
  
void BTnic::logEnd(void) {
  _bufLen = min(_bufLen, BTNIC_BUF_LEN - 2);
  _bufData[_bufLen++] = 0x0D; //Carriage Return
  _bufData[_bufLen++] = 0x0A; //New Line
  _bufCur = 0;
  _state = BTNIC_STATE_TX;
}

int BTnic::getCmdIndex() {
  byte maxValue = pgm_read_byte(CMD_INDEX_MAXVALUE +(_bufData[0] - CMDCODE_MIN));
  if (!maxValue) return 0;
  
  _bufCur = 1;
  char tmpbuf[11];
  while (_bufCur < _bufLen && _bufData[_bufCur] != 0x09) tmpbuf[_bufCur - 1] = _bufData[_bufCur++];

  if (_bufCur == 1) return NO_CMDINDEX;   //Missing Index
  tmpbuf[_bufCur - 1] = '\0';
  int cmdIndex = atoi(tmpbuf);
  if (cmdIndex > maxValue) return NO_CMDINDEX;
  else return cmdIndex;
}
  
byte BTnic::getCmdParamCount() {
  byte paramCount = 0;
  for (byte pos = 0; pos < _bufLen; pos++) if (_bufData[pos] == 0x09) paramCount++;
  return paramCount;
}
  
char* BTnic::getCmdParam(byte paramNum, char *retStr, byte limit) {
  byte pos = 0;
  byte param = 0;
  while (pos < _bufLen && param < paramNum) { if (_bufData[pos++] == 0x09) param++; }
  byte retPos = 0;
  while (pos < _bufLen && _bufData[pos] != 0x09 && retPos < limit) { retStr[retPos++] = _bufData[pos++]; }
  retStr[retPos] = '\0';
  return retStr;
}
  
unsigned long BTnic::getCmdParamNum(byte paramNum) {
  char tmpbuf[20];
  getCmdParam(paramNum, tmpbuf, 10);
  return strtoul(tmpbuf, NULL, 10);
}


/********************************************************************************************************************
 * End of BTnic Class
 ********************************************************************************************************************/

#endif //BTNIC_PROTOCOL
