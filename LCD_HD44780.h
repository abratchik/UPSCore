#ifndef LCD_HD44780_h
#define LCD_HD44780_h

#include "config.h"
#include "utilities.h"

#ifdef DISPLAY_TYPE_LCD_HD44780

#if DISPLAY_SCREEN_HEIGHT > 2
const PROGMEM char DISPLAY_ROW_0[] = "I:   0.0V,  0Hz";
const PROGMEM char DISPLAY_ROW_1[] = "O:   0.0V,  0Hz";
const PROGMEM char DISPLAY_ROW_2[] = "B:   0.0V,  0.00A";
const PROGMEM char DISPLAY_ROW_3[] = "C:  0\% L:  0\%";

const PROGMEM char* const DISPLAY_ROWS[] = {
    DISPLAY_ROW_0,
    DISPLAY_ROW_1,
    DISPLAY_ROW_2,
    DISPLAY_ROW_3
};
#else
const PROGMEM char DISPLAY_ROW_0[]  = "I:  0V O:  0V";
const PROGMEM char DISPLAY_ROW_1[] = "C:  0\% L:  0\%";

const PROGMEM char* const DISPLAY_ROWS[] = {
    DISPLAY_ROW_0,
    DISPLAY_ROW_1
};
#endif

#define DISPLAY_STATUS_OK   " OK"
#define DISPLAY_STATUS_NOK  "NOK"

#include "Display.h"
#include <LiquidCrystal_I2C.h>

class Display : public AbstractDisplay, public LiquidCrystal_I2C {
    public:
        Display(Interactive *lineups, Charger *charger, RMSSensor *vac_in, RMSSensor *vac_out, Sensor *ac_out, Sensor *v_bat, Sensor *c_bat) :
            AbstractDisplay( lineups, charger, vac_in, vac_out, ac_out, v_bat, c_bat ), 
            LiquidCrystal_I2C( DISPLAY_I2C_ADDRESS, DISPLAY_SCREEN_WIDTH, DISPLAY_SCREEN_HEIGHT ) {}; 
    
    public:
        void initialize();

    protected:
        void on_refresh() override ;
        void setup_display() override;
    
    private:
        char _buf[DISPLAY_SCREEN_WIDTH];
        void print_number(float val, int len , int dec, int base = DEC, bool unsgn = false);
        void pgm_print_string(const void* str);

        uint8_t _update_display_rows;
        void update_display_rows();

};

#endif

#endif