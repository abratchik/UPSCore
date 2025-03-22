#ifndef LCD_2004A_h
#define LCD_2004A_h

#include "config.h"

#ifdef DISPLAY_TYPE_LCD_2004A

#include "Display.h"
#include <LiquidCrystal_I2C.h>

class Display : public AbstractDisplay, public LiquidCrystal_I2C {
    public:
        Display(Interactive *lineups, Charger *charger, RMSSensor *vac_in, RMSSensor *vac_out, Sensor *ac_out, Sensor *v_bat) :
            AbstractDisplay( lineups, charger, vac_in, vac_out, ac_out, v_bat ), 
            LiquidCrystal_I2C( 0x27, 20, 4 ) {}; 

        void toggle_display_mode() override  {;};
        void set_display_mode(int mode) override {;};
        int get_display_mode() override { return 0; };
    
    public:
        void initialize();

    protected:
        void on_refresh() override ;
        void setup_display() override;

};

#endif

#endif