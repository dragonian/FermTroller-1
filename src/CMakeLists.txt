# Source files for BrewTroller
set(SOURCE_FILES
  Com_BTnic.h
  Com_BTPD.cpp
  Com.cpp
  Com_S0ASC.cpp
  EEPROM.cpp
  Enum.h
  Events.cpp
  FermCore.cpp
  FermTroller.cpp
  HWProfile.h
  HWProfiles
  Outputs.cpp
  PVOut.h
  Temp.cpp
  UI_LCD.h
  UI.cpp
  Util.cpp)

# Workaround to enable generated Config header to include Enum header
#    *The Config header should get refactored to remove its dependancy on the Enum header*
set(ENUM_HEADER "${CMAKE_CURRENT_LIST_DIR}/Enum.h")

execute_process(COMMAND git describe --tags --abbrev=0
                OUTPUT_VARIABLE VER_STR
                OUTPUT_STRIP_TRAILING_WHITESPACE)

message(STATUS "Building FermTroller version: " ${VER_STR})

####################
# User selectable compile time options
####################
set(board  "OT_EX1" CACHE STRING "FermTroller Hardware Board Target, available options:
BT_10_21                 --> 1.0 - 2.1
BT_22_24                 --> 2.2 - 2.4
BT_3X                    --> 3.X
BT_4P                    --> 4.X Pro
OT_BX1_HERMS             --> BX1 HERMS
OT_BX1_HERMSI2CLCD       --> BX1 HERMS + I2CLCD
OT_BX1_RIMSDF            --> BX1 RIMS / Direct Fired
OT_BX1_RIMSDFI2CLCD      --> BX1 RIMS / Direct Fired + I2CLCD
OT_BX1_SINGLEVESSEL      --> BX1 Single Vessel
OT_BX1_STEAMPWM          --> BX1 Steam or PWM Pump
OT_BX1_STEAMPWMI2CLCD    --> BX1 Steam or PWM Pump + I2CLCD
OT_DX1_HERMS             --> DX1 HERMS
OT_DX1_RIMSDF            --> DX1 RIMS / Direct Fired
OT_DX1_SINGLEVESSEL      --> DX1 Single Vessel
OT_DX1_STEAMPWM          --> DX1 Steam or PWM Pump
OT_EX1                   --> EX1
BT_PHOENIX_HERMS         --> Phoenix HERMS
BT_PHOENIX_RIMS          --> Phoenix RIMS
BT_PHOENIX_SINGLE_VESSEL --> Phoenix Single Vessel
BT_PHOENIX_STEAM_PWM     --> Phoenix Steam or PWM Pump") # Default to DX1 Herms build
add_definitions("-D${board}")
message(STATUS "Building FermTroller for   " ${board})

#####
# Basic Options
#####
option(USEMETRIC "Use Metric Units" OFF)

#####
# Vessel Options
#####
#option(HLT_AS_KETTLE "Use HLT Vessel as Kettle Vessel" OFF)
#option(KETTLE_AS_MASH "Use Kettle Vessel as Mash Vessel" OFF)
#option(SINGLE_VESSEL_SUPPORT "Enable Single Vessel System Configuration" OFF)

#####
# Volume Options
#####
#option(VOLUME_MANUAL "Disable Volume Sensors" OFF)

# Generate Configuration header
configure_file(Config.in.h "${PROJECT_BINARY_DIR}/Config.h")
include_directories(${PROJECT_BINARY_DIR})

#####
# Step Options
#####
#option(AUTO_SKIP_TO_BOIL "Automatically jump programs to Boil Step" OFF)


###################
# Build Setup
###################
add_avr_executable(FermTroller-${board} ${SOURCE_FILES})

# Link libraries
target_link_libraries(FermTroller-${board}.elf BT-libs)
