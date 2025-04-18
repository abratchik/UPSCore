#include "Charger.h"


Charger::Charger(Settings* settings, Sensor* current_sensor, Sensor* voltage_sensor, Print* dbg) {

    set_current_sensor(current_sensor);
    set_voltage_sensor(voltage_sensor);
    _charging_mode = CHARGING_NOT_STARTED;

    pinMode(DEFAULT_CHARGER_PWM_OUT, OUTPUT);

    _settings = settings;
    _dbg = dbg;

    k[CHARGING_KP] = 400.0;
    k[CHARGING_KI] = 0.02;
    k[CHARGING_KD] = 50.0; 
    k[CHARGING_TEST] = 0;

    set_charging(false);       
}

void Charger::start(float current, float voltage, unsigned long ticks) {
    if(_charging) return;

    set_current(current);
    set_voltage(voltage);

    _last_ticks = ticks;
    _last_deviation = 0;
    _deviation_sum = 0;

    set_charging(true);

    _charging_mode = CHARGING_BY_CC;
}

void Charger::set_current(float current) {
    if(current <= 0.0F) {
        _charging_current = 0;
        _charging_mode = CHARGING_TARGET_NOT_SET;
        set_charging(false);
    }
    _charging_current = current;
}

void Charger::regulate(unsigned long ticks) {

    if(!_charging) return;   

    if( !(_charging_mode == CHARGING_BY_CC || _charging_mode == CHARGING_BY_CV || _charging_mode == CHARGING_COMPLETE) )  {
        set_charging(false);
        return;
    }
    
    if( _charging_current <= _cutoff_current || _charging_voltage <= 0.0 ) {
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

    // read sensors
    float reading_v = _voltage_sensor->reading();
    float reading_c = _current_sensor->reading();

    if(reading_v <= _min_battery_voltage) {
        _charging_mode =  CHARGING_BATTERY_DEAD; // battery depleted below minimum, cannot charge
        set_charging(false);
        return;
    }

    // if voltage is reaching target, switch to CV charging 
    float charge_to_full = (float)( _charging_voltage - reading_v )/(_charging_voltage - _min_battery_voltage);    
    if(charge_to_full < 0 && _charging_mode == CHARGING_BY_CC ) {
        _charging_mode = CHARGING_BY_CV;    
    }
    
    float deviation = 0.0F;

    switch(_charging_mode) {
        case CHARGING_BY_CC:
            deviation = (_charging_current - reading_c)/_charging_current;
            break;
        case CHARGING_BY_CV:
            // check if the charging is complete
            if( reading_c <= _cutoff_current ) {
                _charging_mode = CHARGING_COMPLETE;   
                _charging_voltage = INTERACTIVE_STBY_V_BAT;      
            }
    
            deviation = ( _charging_voltage - reading_v )/_charging_voltage;
            break;
        case CHARGING_COMPLETE:
            deviation = ( _charging_voltage - reading_v )/_charging_voltage;
            // if( reading_c > 2.0F * _cutoff_current ) {
            //     _charging_mode = CHARGING_REPLACE_BATTERY;      
            // }
            break;
        default:
            set_charging(false);
            return;
            break;
    }

    long elapsed_ticks = 1; // @todo: switch to actual time?

    // update the integrator component
    _deviation_sum += ( deviation * elapsed_ticks );

    // calculate the regulator output
    _cout_regv = round( k[CHARGING_KP] * deviation + 
                        k[CHARGING_KI] * _deviation_sum + 
                        k[CHARGING_KD] * (deviation - _last_deviation) / elapsed_ticks +
                        k[CHARGING_TEST] );

    // Applying regulator threshold
    if( _cout_regv >= MAXCOUT ) {
        _cout_regv = MAXCOUT;                  
    }
    else if( _cout_regv <= 0 ) {
        _cout_regv = 0;             
    }

    pwmSet10(_cout_regv);

    _last_deviation = deviation;
    _elapsed_ticks = ticks - _last_ticks;
    _last_ticks = ticks;


    return;
}

void Charger::stop() {
    // if(!_charging) return;
    
    _charging_mode = CHARGING_NOT_STARTED;

    _last_deviation = 0.0F;
    _last_ticks = 0;
    
    set_charging(false);

}

void Charger::loadParams() {
    long addr = _settings->getAddr(SETTINGS_CHARGER);

    int num_params = 0;
    EEPROM.get(addr, num_params);

    if( num_params != CHARGING_NUMPARAM ) {
        saveParams();
        return;
    }

    addr += sizeof(int);

    float value;
    for( int p = 0; p < CHARGING_NUMPARAM; p++ ) {
        value = 0;
        EEPROM.get(addr, value);
        k[p] = value;
        addr += sizeof(float);
    }   

    _settings->updateSize( SETTINGS_CHARGER, addr - _settings->getAddr(SETTINGS_CHARGER) );
}

void Charger::saveParams() {
    long addr = _settings->getAddr(SETTINGS_CHARGER);

    EEPROM.put( addr, CHARGING_NUMPARAM );
    addr += sizeof(int);

    for( int p = 0; p < CHARGING_NUMPARAM; p++ ) {
        EEPROM.put( addr, k[p] );
        addr += sizeof(float);
    }

    _settings->updateSize( SETTINGS_CHARGER, addr - _settings->getAddr(SETTINGS_CHARGER) );
}

void Charger::pwmSet10(int value)
{
   OCR1B = value;   
   DDRB |= 1 << 6;  
   TCCR1A |= 0x20;  
}