#include "Sensor.h"


Sensor::Sensor(int pin, float offset, float scale, uint8_t num_samples, uint8_t sampling_period, uint8_t sampling_phase) {
    pinMode(pin, INPUT);
    _pin = pin;
    _num_samples = num_samples;
    _param[SENSOR_PARAM_OFFSET] = offset;
    _param[SENSOR_PARAM_SCALE] = scale;
    _sampling_period = sampling_period;
    _sampling_phase = sampling_phase;

    _readings = (int*) calloc(_num_samples, sizeof(int));

    init();
}

void Sensor::sample() {

    if(!_active) return;

    if(++_sample_counter < _sampling_period ) return;

    _sample_counter = 0;

    int reading = analogRead(_pin);
    
    increment_sum(reading);

    _last_reading = reading;

    _counter++;

    if(_counter >= _num_samples) {
        _counter = 0;
        on_counter_overflow();
    }

}

void Sensor::increment_sum(int reading) {
    int old_reading = *( _readings + _counter );
    _reading_sum += reading - _ready * old_reading;

    *( _readings + _counter) = reading;
}

void Sensor::init() {
    _active = true;
    _counter = 0;
    _reading_sum = 0L;
    _ready = false;
    _avg_reading = 0.0F;
    _sample_counter = _sampling_phase % _sampling_period;
    _last_reading = -1;

    on_init();
}

void Sensor::compute_reading() {
    if(!_ready ) return;
    float total = _reading_sum;
    _avg_reading  = transpose_reading( total / _num_samples  );
}

RMSSensor::RMSSensor(int pin, float offset,  float scale, 
                     uint8_t num_samples, uint8_t sampling_period , uint8_t sampling_phase, uint8_t num_frames) :
    Sensor(pin, offset, scale, num_samples, sampling_period, sampling_phase) {
    _num_frames = num_frames;    
    init();
} 

void RMSSensor::compute_reading() {
    if(!_ready ) return;
    float total_delta_sq = _reading_sum;
    _avg_reading = _period_sum? transpose_reading(sqrtf( total_delta_sq / _period_sum)): 0;  
}

void RMSSensor::increment_sum(int reading) {

    int delta = reading - _median;

    if(_period_start >= 0) {
        _running_sum += square(delta);
    }

    // reading is crossing the median from negative to positive
    if( delta > 0 && (_last_reading > 0) && (_last_reading <= _median ) ) {
        uint16_t period_ptr = _frame_counter * _num_samples + _counter;
        
        if(_period_start >= 0 ) {
            // end of the period reached

            if(period_ptr >= _period_start) 
                _period = period_ptr - _period_start;
            else
                _period = _counter + _num_samples * DEFAULT_NUM_FRAMES - _period_start;

            _running_sum_total += _running_sum;
            _running_period_sum += _period;
        
            _period_counter++;
            _running_sum  = 0L;
        }
        
        _period_start = period_ptr;
    }

    *(_readings + _counter) = delta;

}

void RMSSensor::on_counter_overflow() {


    if(++_frame_counter < _num_frames) return;

    _ready = true;

    _frame_counter = 0;
    // _period_start = -1;
    
    // if no periods were counted, there is no signal or
    // the frequency is too low to measure within the sampling window
    if(!_period_counter) {
        _period_sum = 0;
        _avg_period = 0;
        _avg_reading = 0.0F;
        _period = 0;
        _reading_sum = 0L;
        _running_sum = 0L;
        _running_sum_total = 0L;
        _running_period_sum = 0 ;
        return; 
    }
    
    _period_sum = _running_period_sum;
    _avg_period = (float)_period_sum * _sampling_period / _period_counter;
    
    _running_period_sum = 0;
    _period_counter = 0;

    _reading_sum = _running_sum_total;
    _running_sum_total = 0;

}

void SensorManager::register_sensor(Sensor* sensor) {
    if(_num_sensors >= MAX_NUM_SENSORS) return;

    _sensors[_num_sensors] = sensor;
    _num_sensors++;

}

void SensorManager::sample() {
    if(!_active) return;
    for(uint8_t i=0; i < _num_sensors; i++)
        _sensors[i]->sample();
}

void SensorManager::saveParams() {
    long addr = _settings->getAddr(SETTINGS_SENSORS);

    EEPROM.put(addr, _num_sensors);
    addr += sizeof(int);

    for(uint8_t i=0; i < _num_sensors; i++ ) {
        for( uint8_t p = 0; p < SENSOR_NUMPARAMS; p++ ) {
            EEPROM.put(addr, _sensors[i]->getParam(p));
            addr += sizeof(float);
        }
    }

    _settings->updateSize( SETTINGS_SENSORS, addr - _settings->getAddr(SETTINGS_SENSORS) );
}

void SensorManager::loadParams() {

    long addr = _settings->getAddr(SETTINGS_SENSORS);

    int num_sensors = 0;
    EEPROM.get(addr, num_sensors);

    if( num_sensors != _num_sensors) {
        saveParams();
        return;
    }

    addr += sizeof(int);

    float value;
    for(int i=0; i < _num_sensors; i++ ) {
        for( int p = 0; p < SENSOR_NUMPARAMS; p++ ) {
            value = 0;
            EEPROM.get(addr, value);
            _sensors[i]->setParam( value, p);
            addr += sizeof(float);
        }   
    }

    _settings->updateSize( SETTINGS_SENSORS, addr - _settings->getAddr(SETTINGS_SENSORS) );

}
