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


//*****************************************************************************************************************************
// Special thanks to Jason von Nieda (vonnieda) for the design and code for this cool add-on to BrewTroller.
//*****************************************************************************************************************************


#ifdef BTPD_SUPPORT
#include "Config.h"
#include "Enum.h"
#include "EEPROM.h"

// BTPD_START: Starting address for BTPD data
// BTPD support will use one address per zone

#define BTPD_START 0x20

unsigned long lastBTPD;

void updateBTPD() {
  if (millis() - lastBTPD > BTPD_INTERVAL) {
    for (byte i = 0; i < NUM_ZONES; i++) sendFloatsBTPD(BTPD_START + i, setpoint[i] / 100.0, temp[i] / 100.0);
    lastBTPD = millis();
  }
}

void sendFloatsBTPD(byte chan, float line1, float line2) {
  Wire.beginTransmission(chan);
  Wire.write(0xff);
  Wire.write(0x00);
  Wire.write((uint8_t *) &line1, 4);
  Wire.write((uint8_t *) &line2, 4);
  Wire.endTransmission();
}
#endif
