#ifndef UI_H
#define UI_H

#include <Arduino.h>
#include <menu.h>
#include "Config.h"

void uiInit();
void unlockUI();
void uiCore();
void setActive(byte screen);
void uiEvent(byte eventID, byte eventParam);

unsigned long cfgValveProfile (const char sTitle[], unsigned long defValue);

void adjustLCD();

void assignSensor();
void displayAssignSensorTemp(int sensor);
void cfgOutputs();
void cfgOutput(byte zone, char sTitle[]);

void screenAbout();
void splashScreen();
void menuAlarmZones(); 
void menuAlarms(byte zone);

void menuSetup();

void screenRefresh(byte screen);
void screenInit(byte screen);
void screenEnter(byte screen);



byte scrollMenu(const char* sTitle, menu *objMenu);
void drawMenu(const char* sTitle, menu *objMenu);
byte getChoice(menu *objMenu, byte iRow);
boolean confirmChoice(const char *choice, byte row);
boolean confirmAbort();
boolean confirmDel();
int getTimerValue(const char *sTitle, int defMins, byte maxHours);
void printTimer(byte timer, byte iRow, byte iCol);
void getString(const char *sTitle, char defValue[], byte chars);


long getValue_P(const char *sTitle, long defValue, byte precision, long minValue, long maxValue, const char *dispUnit);
long getValue(char sTitle[], long defValue, byte precision, long minValue, long maxValue, const char *dispUnit);
//unsigned long getValue_P(const char *sTitle, unsigned long defValue, unsigned int divisor, unsigned long maxValue, const char *dispUnit);
//unsigned long getValue(char sTitle[], unsigned long defValue, unsigned int divisor, unsigned long maxValue, const char *dispUnit);

unsigned long ulpow(unsigned long base, unsigned long exponent);
unsigned long getHexValue(char sTitle[], unsigned long defValue);

/**
 * Concatenate two strings from flash, placing result in RAM
 *
 * WARNING: It is not typically more SPACE efficient to concatenate two separate flash strings
 *          than using one static concatenated string. This should only be used to simiplify code
 *          and enhace readability
 */
char* concatPSTRS(char* dst, const char* one, const char* two);

void UIinitEEPROM();

byte ASCII2enc(byte charin);
byte enc2ASCII(byte charin);

#endif