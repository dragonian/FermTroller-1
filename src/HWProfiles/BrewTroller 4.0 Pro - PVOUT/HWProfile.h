/*
BrewTroller 4.0 Pro (PVOUT) Hardware Configuration
BT 4.0 using Pump/Valve Outputs to provide 16 Outputs
*/

#ifndef BT_HWPROFILE
#define BT_HWPROFILE

  #define ENCA_PIN 2
  #define ENCB_PIN 4
  #define ENTER_PIN 5
  
  #define PVOUT_TYPE_MUX
  #define PVOUT_COUNT 16 //16 Outputs
  
  #define MUX_LATCH_PIN 12
  #define MUX_CLOCK_PIN 13
  #define MUX_DATA_PIN 14
  #define MUX_ENABLE_PIN 6
  #define MUX_ENABLE_LOGIC 1

  #define DIGITAL_INPUTS
  #define DIGIN1_PIN 18
  #define DIGIN2_PIN 19
  #define DIGIN3_PIN 20
  #define DIGIN4_PIN 21
  #define DIGIN5_PIN 22
  
  #define UI_LCD_I2C
  #define UI_LCD_I2CADDR 0x01
  #define UI_DISPLAY_SETUP
  #define LCD_DEFAULT_CONTRAST 100
  #define LCD_DEFAULT_BRIGHTNESS 255
  
  #define HEARTBEAT
  #define HEARTBEAT_PIN 0
//**********************************************************************************
// OneWire Temperature Sensor Options
//**********************************************************************************
// TS_ONEWIRE: Enables use of OneWire Temperature Sensors (Future logic may
// support alternatives temperature sensor options.)
#define TS_ONEWIRE
#define TS_ONEWIRE_I2C

// TS_ONEWIRE_PPWR: Specifies whether parasite power is used for OneWire temperature
// sensors. Parasite power allows sensors to obtain their power from the data line
// but significantly increases the time required to read the temperature (94-750ms
// based on resolution versus 10ms with dedicated power).
#define TS_ONEWIRE_PPWR 1

// TS_ONEWIRE_RES: OneWire Temperature Sensor Resolution (9-bit - 12-bit). Valid
// options are: 9, 10, 11, 12). Unless parasite power is being used the recommended
// setting is 12-bit (for DS18B20 sensors). DS18S20 sensors can only operate at a max
// of 9 bit. When using parasite power decreasing the resolution reduces the 
// temperature conversion time: 
//   12-bit (0.0625C / 0.1125F) = 750ms 
//   11-bit (0.125C  / 0.225F ) = 375ms 
//   10-bit (0.25C   / 0.45F  ) = 188ms 
//    9-bit (0.5C    / 0.9F   ) =  94ms   
#define TS_ONEWIRE_RES 11

// DS2482_ADDR: I2C Address of DS2482 OneWire Master (used for TS_OneWire_I2C)
// Should be 0x18, 0x19, 0x1A, 0x1B
#define DS2482_ADDR 0x1B
//**********************************************************************************

#endif
