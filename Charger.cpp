#include "Charger.h"


Charger::Charger(Sensor* current_sensor, Sensor* voltage_sensor, int cout_pin, int charging_pin) {
    _cout_pin = cout_pin;
    _charging_pin = charging_pin;

    set_current_sensor(current_sensor);
    set_voltage_sensor(voltage_sensor);
    _charging_mode = CHARGING_NOT_STARTED;

    pinMode(_cout_pin, OUTPUT);
    pinMode(_charging_pin, OUTPUT);  

    set_charging(false);       
}

void Charger::start(float current, float voltage) {
    if(_charging) return;

    set_current(current);
    set_voltage(voltage);

    set_charging(true);

    _charging_mode = CHARGING_BY_CC;
}

void Charger::set_current(float current) {
    if(current <= 0.0F) {
        _charging_current = 0;
        regulate();
    }
    _charging_current = current;
}

void Charger::regulate() {
    float deviation;
    int step = 1; 
    float reading_c, reading_v;

    if(!_charging) return;   
        
    
    if( _charging_current <= 0.0 || _charging_voltage <= 0.0 ) {
        _cout_regv = 0;
        analogWrite(_cout_pin, _cout_regv);
        _charging_mode = CHARGING_TARGET_NOT_SET;
        set_charging(false);
        return;                          
    }
    if(_current_sensor == NULL) {
        _charging_mode = CHARGING_C_SENSOR_NOT_SET; 
        set_charging(false);
        return;
    } 
    if(!_current_sensor->ready()) {
        _charging_mode = CHARGING_C_SENSOR_NOT_READY;
        return; 
    }    
    if(_voltage_sensor == NULL) {
        _charging_mode =  CHARGING_V_SENSOR_NOT_SET;
        set_charging(false);
        return;
    }      
    if(!_voltage_sensor->ready()) {
        _charging_mode =  CHARGING_V_SENSOR_NOT_READY;
        return;
    }    

    // for overvoltage protection, we start checking voltage first
    reading_v = _voltage_sensor->reading();

    if(reading_v <= _min_charge_voltage) {
        _charging_mode =  CHARGING_BATTERY_DEAD; // battery fully depleted, cannot charge
        set_charging(false);
        return;
    }

    deviation = (float)( _charging_voltage - reading_v )/_charging_voltage;
    
    if(deviation < 0) {
        _charging_mode = CHARGING_BY_CV;       // if voltage is over to threshold, switch to CV charging 
    // } else if(deviation > 0 and int(_current_sensor->reading()) > _charging_current) {
    //     _charging_mode = CHARGEBY_CC;       // if current is over the CC, switch to CC charging 
    }
    
    reading_c = _current_sensor->reading();

    // if charging mode is CC, define the reading and deviation
    if(_charging_mode == CHARGING_BY_CC) {
        deviation = (_charging_current - reading_c)/_charging_current;
    }
    else if(_charging_mode == CHARGING_BY_CV) {
        // check if the charging is complete
        if(reading_c <= _cutoff_current ) {
            _cout_regv = 0;
            analogWrite(_cout_pin, _cout_regv);
            _charging_mode = CHARGING_COMPLETE;
            set_charging(false);
            return ;                              
        }
        // check if current is over the CC, regulate current
        else if(reading_c > (float)_charging_current)  {
            deviation = ((float)_charging_current - reading_c)/(float)_charging_current;
        }
    }

    // calculate the step based on the deviation
    step = exp(abs(deviation*100)* 0.04);
    
    _cout_regv += sgn(deviation) * step;

    // Checking regulator threshold
    if( _cout_regv > MAXCOUT ) {
        _cout_regv = MAXCOUT;
        _charging_mode = CHARGING_MAX_HIT;                       
    }
    else if( _cout_regv < 0 ) {
        _cout_regv = 0;
        _charging_mode = CHARGING_MIN_HIT;                
    }

    analogWrite(_cout_pin, _cout_regv);

    return;
}

void Charger::stop() {
    if(!_charging) return;

    set_current(0);
    
    _charging_mode = CHARGING_NOT_STARTED;
    
    set_charging(false);

}
