#ifndef Config_h
#define Config_h

#include <Arduino.h>

#define MAX_NUM_SENSORS 5
#define SENSOR_INPUT_VAC_IN A0        // input AC voltage
#define SENSOR_OUTPUT_VAC_IN A1       // output AC voltage
#define SENSOR_OUTPUT_C_IN A2         // output AC current sensor
#define SENSOR_BAT_V_IN A3            // battery voltage sensor input
#define SENSOR_BAT_C_IN A7            // battery current sensor input

#define BUZZ_PIN 3                    // beeper output pin
#define RESET_PIN 4                   // the pin used to trigger reset. Requires 
                                      // 100nF capacitor between this pin and
                                      // the Reset pin of the board.
                                      // Reset can be triggered by the 'R' command

#define INTERACTIVE_INPUT_RLY_OUT 5   // RY1 relay manage pin
#define INTERACTIVE_OUTPUT_RLY_OUT 6  // RY4 relay manage pin
#define INTERACTIVE_LEFT_RLY_OUT 7    // RY2 relay manage pin
#define INTERACTIVE_RIGHT_RLY_OUT 8   // RY3 relay manage pin
#define INTERACTIVE_INVERTER_OUT 9    // inverter manage pin
#define INTERACTIVE_ERROR_OUT LED_BUILTIN

#define TIMER_ONE_SEC   1000          // number of ticks to form 1 second
#define MAX_NUM_TIMERS  5             // number of timers used

#define INTERACTIVE_DEFAULT_INPUT_VOLTAGE 230.0F    // nominal input VAC 
#define INTERACTIVE_INPUT_VOLTAGE_DEVIATION 0.08F   // max input VAC deviation
#define INTERACTIVE_MAX_SINE_DEVIATION  150.0F      // max deviation in volts from the ideal sine waveform
#define INTERACTIVE_INPUT_VOLTAGE_HYSTERESIS 0.02F  // input VAC hysteresis
#define INTERACTIVE_MAX_AC_OUT 4.0F                 // max output current, Amp
#define INTERACTIVE_MIN_AC_OUT 0.1F
#define INTERACTIVE_MAX_V_BAT_CELL 14.4F            // max voltage per cell in cycle use
#define INTERACTIVE_STBY_V_BAT_CELL 13.6F           // max voltage per cell in standby use
#define INTERACTIVE_MIN_V_BAT_CELL 10.5F
#define INTERACTIVE_NUM_CELLS  2                    // number of cells in a battery pack (serial connection)
#define INTERACTIVE_NUM_BATTERY_PACKS  1            // number of battery packs
#define INTERACTIVE_BATTERY_AH 9.0F                 // battery cell capacity in AH
#define INTERACTIVE_BATTERY_LOW 0.2F                // battery is low
#define INTERACTIVE_BATTERY_CRITICAL 0.1F           // battery is critically low
#define INTERACTIVE_DEFAULT_FREQ 50.0F

#define SELF_TEST_MIN_BAT_LVL 0.8F                  // minimum required battery charge level for the selftest to run

#define DISPLAY_DA_OUT   11
#define DISPLAY_CLK_OUT  13

// uncomment only one line for enabling display of the supported type
// #define DISPLAY_TYPE_NONE          // no display 
// #define DISPLAY_TYPE_LED_TM1640    // LED assembly display based on TM1640
// #define DISPLAY_TYPE_LCD_HD44780    // 16x2 or 20x4 LCD matrix display based on HD44780

// allows to configure display I2C address and resolution for HD44780
#ifdef DISPLAY_TYPE_LCD_HD44780
#define DISPLAY_I2C_ADDRESS     0x27
#define DISPLAY_SCREEN_WIDTH    20
#define DISPLAY_SCREEN_HEIGHT   4
#endif

#define DISPLAY_MAX_BRIGHTNESS 4        // maximum brightness level of the display backlit
#define DISPLAY_DEFAULT_BRIGHTNESS 1    // default brightness level of the display backlit

#define SERIAL_MONITOR_BAUD_RATE 9600

// the following constants are arbitrary and can be updated as necessary for a particular 
// UPS implementation
const PROGMEM char MANUFACTURER[] = "SmartEyeAI";
const PROGMEM char PART_NUMBER[] = "SPSLUPS2000";
const PROGMEM char PART_MODEL[] = "SmartEyeAI LineUPS-2000.PureSine.ATM328P.USB.2U";
#define RATED_VA 2000
#define ACTUAL_VA 1200
const PROGMEM char FIRMWARE_VERSION[] = "001.00";

// fully drained battery voltage
const static float INTERACTIVE_MIN_V_BAT = INTERACTIVE_MIN_V_BAT_CELL * INTERACTIVE_NUM_CELLS;    
// max battery voltage             
const static float INTERACTIVE_MAX_V_BAT = INTERACTIVE_MAX_V_BAT_CELL * INTERACTIVE_NUM_CELLS; 
// standby battery voltage (fully charged)
const static float INTERACTIVE_STBY_V_BAT = INTERACTIVE_STBY_V_BAT_CELL * INTERACTIVE_NUM_CELLS; 
// fully charged minus fully drained battery voltage 
const static float INTERACTIVE_V_BAT_DELTA = INTERACTIVE_STBY_V_BAT - INTERACTIVE_MIN_V_BAT;
// full battery capacity in AH
const static float INTERACTIVE_TOTAL_BATTERY_CAP = INTERACTIVE_BATTERY_AH * 
                                                   INTERACTIVE_NUM_CELLS * 
                                                   INTERACTIVE_NUM_BATTERY_PACKS;

#endif