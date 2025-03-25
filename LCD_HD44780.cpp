#include "LCD_HD44780.h"

#ifdef DISPLAY_TYPE_LCD_HD44780

void Display::initialize() {
    LiquidCrystal_I2C::init();
    setup_display();
    clear();
    printstr(DISPLAY_ROW_0);
    setCursor(0,1);
    printstr(DISPLAY_ROW_1); 
#if DISPLAY_SCREEN_HEIGHT > 2 
    setCursor(0,2);
    printstr(DISPLAY_ROW_2);
    setCursor(0,3);
    printstr(DISPLAY_ROW_3);
#endif    
};

void Display::setup_display() {
    if(_active) {
        display();
    }
    else {
        noDisplay();
    }

    if(_brightness) {
        backlight();
    }
    else {
        noBacklight();
    }
};

void Display::on_refresh() {
#if DISPLAY_SCREEN_HEIGHT > 2 
    setCursor(5,0); print_number(_vac_in->reading(), 4,1);
    setCursor(14,0); print_number(round(_vac_in->get_period()>0? (float)TIMER_ONE_SEC/_vac_in->get_period() : 0), 3,0);
    setCursor(5,1); print_number(_vac_out->reading(), 4,1); 
    setCursor(14,1); print_number(round(_vac_out->get_period()>0? (float)TIMER_ONE_SEC/_vac_out->get_period() : 0), 3,0);
    setCursor(5,2); print_number(_v_bat->reading(), 3,1);
    setCursor(12,2); print_number(_c_bat->reading(), 5,2); 
    setCursor(2,3); print_number(round((float)_lineups->getBatteryLevel() * 100.00) , 3, 0 );
    setCursor(9,3); print_number(round((float) 100.00 * _ac_out->reading() / INTERACTIVE_MAX_AC_OUT) , 3, 0 );
    setCursor(14,3); print_number( _lineups->getStatus(), 2, 0, HEX, true);
    setCursor(17,3); print_number( HEX * _charger->is_charging() +  _charger->get_mode(), 2, 0, HEX, true);
#else
    setCursor(2,0); print_number(_vac_in->readingR(),3,0);
    setCursor(9,0); print_number(_vac_out->readingR(),3,0);
    setCursor(14,0); print_number( _lineups->getStatus(), 2, 0, HEX, true);
    setCursor(2,1); print_number((float)_lineups->getBatteryLevel() * 100.00 , 3, 0 );
    setCursor(9,1); print_number(round((float) 100.00 * _ac_out->reading() / INTERACTIVE_MAX_AC_OUT) , 3, 0 );
    setCursor(14,1); print_number( HEX * _charger->is_charging() +  _charger->get_mode(), 2, 0, HEX, true);
#endif
};

void Display::print_number(float val, int len, int dec, int base, bool unsgn) {
    ex_print_number_to_buf(_buf, val, len, dec, base, unsgn);
    print(_buf);
}

#endif