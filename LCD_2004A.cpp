#include "LCD_2004A.h"

#ifdef DISPLAY_TYPE_LCD_2004A

void Display::initialize() {
    LiquidCrystal_I2C::init();
    setup_display();
    clear();
    printstr("Inp: 220.2VAC, 50Hz");
    setCursor(0,1);
    printstr("Out: 220.2VAC, 50Hz");  
    setCursor(0,2);
    printstr("Bat: 24.6V, -11.23A");
    setCursor(0,3);
    printstr("B:100\% L:002\% E000");
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
    setCursor(0,5);
    print(_vac_in->reading(), 1);
};

#endif