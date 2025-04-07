#include "LCD_HD44780.h"

#ifdef DISPLAY_TYPE_LCD_HD44780

void Display::initialize() {
    LiquidCrystal_I2C::init();
    setup_display();
    clear();

    setCursor(5,1);ex_print_str_to_stream(this, MANUFACTURER, true);

    _update_display_rows = true;
 
};

void Display::setup_display() {
    if(!_active) {
        noDisplay();
        noBacklight();
    }
    else {
        display();
        if(_brightness) {
            backlight();
        }
        else {
            noBacklight();
        }
    }


};

void Display::on_refresh() {

    update_display_rows();

    uint16_t status = _lineups->getStatus();

#if DISPLAY_SCREEN_HEIGHT > 2 
    setCursor(3,0); print_number(_vac_in->reading(), 4,1);
    setCursor(10,0); print_number(_vac_in->get_frequency() , 3,0);
    setCursor(16,0);
    if(!bitRead(status, UTILITY_FAIL)) 
        printstr(DISPLAY_STATUS_OK);
    else
        printstr(DISPLAY_STATUS_NOK);
        
    setCursor(3,1); print_number(_vac_out->reading(), 4,1); 
    setCursor(10,1); print_number(_vac_out->get_frequency(), 3,0);
    setCursor(16,1);
    if(!bitRead(status, UPS_FAULT)) 
        printstr(DISPLAY_STATUS_OK);
    else 
        printstr(DISPLAY_STATUS_NOK);

    setCursor(3,2); print_number(_v_bat->reading(), 5,2);
    setCursor(11,2); print_number(_c_bat->reading(), 5,2); 
    setCursor(2,3); print_number(round((float)_lineups->getBatteryLevel() * 100.00) , 3, 0 );
    setCursor(9,3); print_number(round((float) 100.00 * _ac_out->reading() / INTERACTIVE_MAX_AC_OUT) , 3, 0 );
    setCursor(14,3); print_number( status, 2, 0, HEX, true);
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

void Display::pgm_print_string(const void* pgm_str) {
    strcpy_P( _buf, (char*) pgm_read_ptr(pgm_str) );
    printstr(_buf);
}

void Display::update_display_rows() {
    if(!_update_display_rows) return;
    for(uint8_t i=0; i< DISPLAY_SCREEN_HEIGHT; i++) {
        setCursor(0,i);
        pgm_print_string( &( DISPLAY_ROWS[i] ) ) ;
    }
    _update_display_rows = false;
}

#endif