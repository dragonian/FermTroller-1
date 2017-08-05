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
#include "Util.h"

void strLPad(char retString[], byte len, char pad) {
  char strVal[len + 1];
  strcpy(strVal, retString);
  memset(retString, pad, len);
  retString[len - strlen(strVal)] = '\0';
  strcat(retString, strVal);
}

//Converts a "Virtual Float" (fixed decimal value represented in tenths, hundredths, thousandths, etc.) to a string
void vftoa(long val, char retStr[], byte precision, boolean decimal) {
  char lbuf[12] = "";
  ltoa(val, lbuf, 10);
  int wholeDigits = strlen(lbuf) - precision;
  if (wholeDigits > 0) strlcpy(retStr, lbuf, wholeDigits + 1);
  else strcpy(retStr, "0");
  if (precision) {
    if (decimal) strcat(retStr, ".");
    wholeDigits = max(wholeDigits, 0);
    strLPad(lbuf + wholeDigits, precision, '0');
    strcat(retStr, lbuf + wholeDigits);
  }
}

//Truncate a string representation of a float to (length) chars but do not end string with a decimal point
void truncFloat(char retStr[], byte len) {
  retStr[len] = '\0';
  if (retStr[len - 1] == '.') retStr[len - 1] = '\0';
}

unsigned long pow10(byte power) {
  unsigned long retValue = 1;
  for (byte i = 0; i < power; i++) retValue *= 10;
  return retValue;
}
