#ifndef Display_h
#define Display_h

#include "config.h"

#include "Interactive.h"
#include "Charger.h"
#include "Sensor.h"

class AbstractDisplay {
    public:
        AbstractDisplay(Interactive *lineups, Charger *charger, RMSSensor *vac_in, RMSSensor *vac_out, Sensor *ac_out, Sensor *v_bat) {
                // link to Interactive
            _lineups = lineups;

            // link to Charger
            _charger = charger;

            // link to sensors
            _vac_in = vac_in;
            _vac_out = vac_out;
            _ac_out = ac_out;
            _v_bat = v_bat;
            
            _active = true;
            _refresh = false;

            _brightness = DISPLAY_DEFAULT_BRIGHTNESS;
        };

        void initialize() {;};

        void toggle() { _active = !_active; setup_display(); };

        void set_brightness(int brightness) { _brightness = brightness; setup_display(); }

        virtual void toggle_display_mode()=0;
        virtual void set_display_mode(int mode)=0;
        virtual int get_display_mode()=0;

        void init_refresh(){ _refresh = true;};
        void refresh() {
            if(!_refresh) return;
            on_refresh();
            _refresh = false;
        };

    protected:
        virtual void on_refresh(){;};

        virtual void setup_display(){;};

        Interactive *_lineups;
        Charger *_charger;
        RMSSensor *_vac_in, *_vac_out;
        Sensor *_ac_out, *_v_bat;

        bool _active;
        bool _refresh;

        int _brightness;

};

#ifdef DISPLAY_TYPE_LED_TM1640
#include "LED_TM1640.h"
#endif

#ifdef DISPLAY_TYPE_LCD_2004A
#include "LCD_2004A.h"
#endif 

// stub for dummy box (no display)
#ifdef DISPLAY_TYPE_NONE
class Display : public AbstractDisplay {
    public:
        using AbstractDisplay::AbstractDisplay;
};
#endif


#endif