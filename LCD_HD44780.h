#ifndef LCD_HD44780_h
#define LCD_HD44780_h

#include "config.h"
#include "utilities.h"

#ifdef DISPLAY_TYPE_LCD_HD44780

#if DISPLAY_SCREEN_HEIGHT > 2
#define DISPLAY_ROW_0 "Inp:   0.0VAC,  0Hz"
#define DISPLAY_ROW_1 "Out:   0.0VAC,  0Hz"
#define DISPLAY_ROW_2 "Bat:  0.0V,   0.00A"
#define DISPLAY_ROW_3 "C:  0\% L:  0\%"
#else
#define DISPLAY_ROW_0 "I:  0V O:  0V"
#define DISPLAY_ROW_1 "C:  0\% L:  0\%"
#endif


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
        char _buf[7];
        void print_number(float val, int len , int dec, int base = DEC, bool unsgn = false);

};

#endif

#endif