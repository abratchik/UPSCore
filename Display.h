#ifndef Display_h
#define Display_h

#include "config.h"

#include <TM1640.h>   

#define DISPLAY_MAX_POS         8   // max number of used groups  

enum ReadingUnit {
    UNIT_HEX,
    UNIT_HZ,
    UNIT_VAC
};

enum ReadingDirection {
    LEVEL_INCREASING = 1,
    LEVEL_NO_CHANGE = 0,
    LEVEL_DECREASING = -1
};

enum DisplayFlag {
    BATTERY_MODE_INDICATOR = 16,
    UNUSUAL_MODE_INDICATOR = 64,
    LOAD_INDICATOR = 256,
    BATTERY_INDICATOR = 4096,
    AC_MODE_INDICATOR = 8192,
    UPS_FAULT_INDICATOR = 16384
};

enum DisplayGroups {
    INPUT_RELAY_GRID,
    INPUT_FREQ_GRID,
    INPUT_VAC_GRID,
    OUTPUT_RELAY_GRID,
    OUTPUT_FREQ_GRID,
    OUTPUT_VAC_GRID,
    FLAG_LO_GRID,
    FLAG_HI_GRID
};

const int DISPLAY_LEVEL_N_SEGMENTS = 4;
const float DISPLAY_LEVEL_INTERVAL = (float) 1.0 / DISPLAY_LEVEL_N_SEGMENTS;

// definition for standard hexadecimal numbers
const PROGMEM uint8_t DISPLAY_NUMBER_FONT[] = {
  0b11111010, // 0
  0b00001010, // 1
  0b10111100, // 2
  0b10011110, // 3
  0b01001110, // 4
  0b11010110, // 5
  0b11110110, // 6
  0b10001010, // 7
  0b11111110, // 8
  0b11011110, // 9
  0b11101110, // A
  0b01110110, // B
  0b11110000, // C
  0b00111110, // D
  0b11110100, // E
  0b11100100  // F
};

const PROGMEM uint16_t DISPLAY_LOAD_LEVEL[] = {
  0b0000000000000001, // 25%
  0b0000000000001000, // 50%
  0b0000000000000100, // 75%
  0b0000000000000010  // 100%
};

const PROGMEM uint16_t DISPLAY_BATTERY_LEVEL[] = {
  0b1000000000000000, // 25%
  0b0000100000000000, // 50%
  0b0000010000000000, // 75%
  0b0000001000000000  // 100%
};

class Display : public TM1640 {
    public:
        Display(); 

        // Setting the input reading (VAC or frequency)
        void setInputReading( int reading, ReadingUnit mode = UNIT_VAC );

        // Setting the output reading (VAC or frequency)
        void setOutputReading( int reading, ReadingUnit mode = UNIT_VAC );

        // Setting the battery level bits in the board[FLAG_HI_GRID]
        // @param level Sets the normalized battery charge level (0.0 - empty, 1.0 - fully charged)
        // @param direction used for blinking the last segment during charge/discharge
        void setBatteryLevel(float level, ReadingDirection direction = LEVEL_NO_CHANGE);

        // Setting the load level bits in the board[FLAG_LO_GRID]
        void setLoadLevel(float level);

        void setInputRelayStatus(float rly_status) { setRelayStatus(rly_status, 0); };

        void setOutputRelayStatus(float rly_status) { setRelayStatus(rly_status, 3); };

        void show(bool blink_state = false);
        
        void clear(bool clear_display = true);
        
        void setFlag(DisplayFlag flag);

        void setBlink(DisplayFlag flag);

        virtual void setupDisplay(boolean active, byte intensity) override;

        bool isActive() { return _active; };

        void toggle();
    
    protected:

        void setReading(int reading, ReadingUnit mode = UNIT_VAC, int start = 2, int stop = 0);

        // display the level of the load or the battery
        // @param level - level to be displayed. Can be any float from 0 to 1
        // @param isHI - if true, battery level is displayed, else load
        // @param direction - direction of the change of the level
        void setLevel(float level, bool isHI = false, ReadingDirection direction = LEVEL_NO_CHANGE);

        void setRelayStatus(bool rly_status, uint8_t grid = 0 );

    private:

        uint8_t board[8];   // array to store display data for each group
        uint8_t blink[8];    // array to store blink mask for each group

        bool _active;
        int _brightness;
       
};

#endif