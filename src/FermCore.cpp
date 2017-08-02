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
#include "HWProfile.h"

void fermCore() {
  #ifdef HEARTBEAT
    heartbeat();
  #endif
  
  #ifndef NOUI
    LCD.update();
  #endif
  
  //Temps: Temp.pde
  updateTemps();
 
  //Alarm logic: Outputs.pde
  updateAlarm();

  //Outputs: Outputs.pde
  processOutputs();
  
  //Set Valve Outputs based on active valve profiles (if changed): Outputs.pde
  updateValves();
  
  updateCom();
}

#ifdef HEARTBEAT
  unsigned long hbStart = 0;
  void heartbeat() {
    unsigned long now = millis();
    if (now < hbStart) hbStart = 0; //Timer overflow occurred
    if (now - hbStart > 750) {
      hbPin.toggle();
      hbStart = now;
    }
  }
#endif
