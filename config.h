#ifndef Config_h
#define Config_h

#include <Arduino.h>

#define MAX_NUM_SENSORS 5
#define SENSOR_INPUT_VAC_IN A0
#define SENSOR_OUTPUT_VAC_IN A1
#define SENSOR_OUTPUT_C_IN A2
#define SENSOR_BAT_V_IN A3
#define SENSOR_BAT_C_IN A4

#define RESET_PIN 4

#define INTERACTIVE_INPUT_RLY_OUT 5
#define INTERACTIVE_OUTPUT_RLY_OUT 6
#define INTERACTIVE_LEFT_RLY_OUT 7
#define INTERACTIVE_RIGHT_RLY_OUT 8
#define INTERACTIVE_INVERTER_OUT 9

#define BEEPER_OUT 12

#define TIMER_ONE_SEC   20            // number of ticks to form 1 second
#define MAX_NUM_TIMERS  5             // number of timers used

#define INTERACTIVE_DEFAULT_INPUT_VOLTAGE 220.0F
#define INTERACTIVE_INPUT_VOLTAGE_DEVIATION 0.08F
#define INTERACTIVE_INPUT_VOLTAGE_HYSTERESIS 0.02F
#define INTERACTIVE_MAX_AC_OUT 4.0F
#define INTERACTIVE_MIN_AC_OUT 0.1F
#define INTERACTIVE_MAX_V_BAT_CELL 13.8F
#define INTERACTIVE_MIN_V_BAT_CELL 10.5F
#define INTERACTIVE_NUM_CELLS  2
#define INTERACTIVE_NUM_BATTERY_PACKS  1
#define INTERACTIVE_BATTERY_AH 9.0F                 // battery cell capacity in AH
#define INTERACTIVE_BATTERY_LOW 0.1F
#define INTERACTIVE_DEFAULT_FREQ 50.0F

#define SELF_TEST_MIN_BAT_LVL 0.8F

#define DISPLAY_DA_OUT   11
#define DISPLAY_CLK_OUT  13
#define DISPLAY_BLINK_FREQ      16
#define DISPLAY_MAX_BRIGHTNESS 4
#define DISPLAY_DEFAULT_BRIGHTNESS 1

#define SERIAL_MONITOR_BAUD_RATE 9600

#define MANUFACTURER "ExeGate"
#define PART_NUMBER "EX293851RUS"
#define PART_MODEL "ExeGate ServerRM UNL-2000.LCD.AVR.2SH.3C13.USB.2U"
#define RATED_VA 2000
#define ACTUAL_VA 1200
#define FIRMWARE_VERSION "001.00"

// fully drained battery voltage
const static float INTERACTIVE_MIN_V_BAT = INTERACTIVE_MIN_V_BAT_CELL * INTERACTIVE_NUM_CELLS;    
// fully charged battery voltage             
const static float INTERACTIVE_MAX_V_BAT = INTERACTIVE_MAX_V_BAT_CELL * INTERACTIVE_NUM_CELLS; 
// fully charged minus fully drained battery voltage 
const static float INTERACTIVE_V_BAT_DELTA = INTERACTIVE_MAX_V_BAT - INTERACTIVE_MIN_V_BAT;
// full battery capacity in AH
const static float INTERACTIVE_TOTAL_BATTERY_CAP = INTERACTIVE_BATTERY_AH * 
                                                   INTERACTIVE_NUM_CELLS * 
                                                   INTERACTIVE_NUM_BATTERY_PACKS;

#endif