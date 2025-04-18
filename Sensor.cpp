#include "Sensor.h"


Sensor::Sensor(int pin, float offset, float scale, uint8_t num_samples, uint8_t sampling_period, uint8_t sampling_phase) {
    pinMode(pin, INPUT);
    _pin = pin;
    _num_samples = num_samples;
    setParam(offset, SENSOR_PARAM_OFFSET);
    setParam(scale, SENSOR_PARAM_SCALE);
    _sampling_period = sampling_period;
    _sampling_phase = sampling_phase;

    init();
}

void Sensor::sample() {

    if(!_active) return;

    if(  ++_sample_counter % _sampling_period  ) return;

    int reading = analogRead(_pin);
    
    increment_sum(reading);

    _last_reading = reading;

    _counter++;

    if( _counter >= _num_samples ) {
        _counter = 0;
        on_counter_overflow();
    }

}

void SimpleSensor::increment_sum(int reading) {
    int old_reading = *( _readings + _counter );
    _reading_sum += reading - _ready * old_reading;

    *( _readings + _counter) = reading;
}

void Sensor::init() {
    on_init();
    reset();
}

void Sensor::reset() {
    _counter = 0;
    _reading_sum = 0L;
    _avg_reading = 0.0F;
    _sample_counter = _sampling_phase % _sampling_period;
    _last_reading = NOT_DEFINED; // this is to indicate that the sensor has not been sampled yet
}

void Sensor::print() {
    if(!_stream) return;      
    ex_printf_to_stream(_stream, "%.5f %.5f %f %i",
        _param[SENSOR_PARAM_OFFSET], 
        _param[SENSOR_PARAM_SCALE],
        _avg_reading, 
        _last_reading);
}

SimpleSensor::SimpleSensor(int pin, float offset,  float scale, uint8_t num_samples, uint8_t sampling_period , uint8_t sampling_phase) :
    Sensor(pin, offset, scale, num_samples, sampling_period, sampling_phase) {
    init();
} 

void SimpleSensor::reset() {
    Sensor::reset();
    memset(_readings, 0x0, _num_samples * sizeof(int));
}

void SimpleSensor::on_init() {
    Sensor::on_init();
    _readings = (int*) calloc(_num_samples, sizeof(int)); 
}

void SimpleSensor::compute_reading() {
    if(!_ready ) return;
    float total = _reading_sum;
    _avg_reading  = transpose_reading( total / _num_samples  );
}

void SimpleSensor::dump() {
    if(!_stream) return;

    for(int i=0; i < _num_samples; i++) {
        if(i) _stream->write(',');
        _stream->print(*(_readings+i)); 
    }
}

RMSSensor::RMSSensor(int pin, float offset,  float scale, 
                     uint8_t num_samples, uint8_t sampling_period , uint8_t sampling_phase, uint16_t num_periods) :
    Sensor(pin, offset, scale, num_samples, sampling_period, sampling_phase) {
    _num_periods = num_periods;    
    init();
} 

void RMSSensor::reset() {
    Sensor::reset();

    _avg_period = 0.0F;
    _avg_frequency = 0.0F;
    _bad_sine = false;
    _last_amplitude = 0;
    _running_max_delta = 0;
    _period_index = 0;
    _period_counter = 0;
    _period_tick_counter = 0;
    _period_sum = 0;
    _period_start = NOT_DEFINED;
    _running_sum  = 0L;
    _running_median_error = 0;
    _median_error = 0;

    memset(_sq_deltas, 0x0, _num_periods * sizeof(long));
    memset(_periods, 0x0, _num_periods * sizeof(int));
    // memset(_deltas, 0x0, 20 * sizeof(int) );
}

void RMSSensor::on_init() {
    Sensor::on_init();
    // _deltas = (int*) calloc( 20 , sizeof(int));
    _sq_deltas = (long*) calloc( _num_periods , sizeof(long)); 
    _periods = (int*) calloc( _num_periods , sizeof(int)); 

}

void RMSSensor::compute_reading() {
    if(!_ready ) return;
    float total_delta_sq = _reading_sum;
    int timeframe = _period_sum * _sampling_period;
    _avg_reading = _period_sum? transpose_reading(sqrtf( total_delta_sq / timeframe )): 0;  
    _avg_period = (float) timeframe / _num_periods ;
    _avg_frequency = _avg_period?  (float) TIMER_ONE_SEC / _avg_period  : 0 ;
}

// This method updates the running sum of squared deltas and tracks the period of the signal.
// It calculates the difference (delta) between the current reading and the median, 
// and updates the running sum of squared deltas. Additionally, it detects when the signal 
// crosses the median from negative to positive, marking the start and end of a period.
void RMSSensor::increment_sum(int reading) {

    int delta = reading - _median;
    _running_max_delta = max(_running_max_delta, abs(delta));

    if(_period_start != NOT_DEFINED) {
        _running_sum += square(delta);
        _period_tick_counter++;
        if(_calibrate) _running_median_error += delta;
    }

    // skiping first reading
    if( _last_reading != NOT_DEFINED ) {

        int angle = (int)round( _period_tick_counter * _avg_frequency * SENSOR_GRAD_PER_SEC );
        _expected_delta =  (float)_last_amplitude * ex_fast_sine( angle );
        _bad_sine = abs((float) delta - _expected_delta) >  _max_delta_deviation;

        // reading is crossing the median from negative to positive
        if( delta > 0  &&  _last_reading <= _median ) {
            
            // end of the period reached
            if(_period_start != NOT_DEFINED ) {
                
                long old_reading = *(_sq_deltas + _period_index);
                int old_period = *(_periods + _period_index);

                _reading_sum += _running_sum - old_reading;
                _period_sum += _period_tick_counter - old_period;

                *(_sq_deltas + _period_index) = _running_sum;
                *(_periods + _period_index) = _period_tick_counter;
                
                if(_calibrate) {
                    _median_error = (int) _running_median_error / _period_tick_counter;
                    _running_median_error = 0;
                }

                _last_amplitude = _running_max_delta;
                _running_max_delta = 0;

                _running_sum  = 0L;
                _period_tick_counter = 0;
            
                _period_index++;
                _period_counter++;

                _bad_sine = false;

                if(_period_index >= _num_periods) {
                    _period_index = 0;
                    // once the required number of periods received, the sensor is ready
                    _ready = true;
                }
            }
            
            _period_start = _counter;
        }
    }
}

void RMSSensor::on_counter_overflow() {
    
    // sensor is also ready once the end of sampling window is reached
    _ready = true;

    // if no periods were detected during the _num_samples, there is no signal or
    // the frequency is too low or too high to measure for the given sampling window (_num_samples * _sampling_period )
    if(!_period_counter || _period_counter > (int)(_num_samples >> 1) ) {
        reset();
    }

    _period_counter = 0;

}

void RMSSensor::dump() {
    if(!_stream) return;

    for(int i=0; i<_num_periods; i++) {
        if(i) _stream->write(';');
        _stream->print(*(_sq_deltas+i)); 
        _stream->write(',');
        _stream->print(*(_periods+i)); 
    }

    _stream->write(' ');
    _stream->print(_last_amplitude);
    _stream->write(' ');
    _stream->print(_last_reading - _median);
    _stream->write(' ');
    _stream->print(_expected_delta);
    _stream->write(' ');
    _stream->print(_bad_sine);

}

void RMSSensor::print() {

    if(!_stream ) return;
    
    Sensor::print();

    if(_calibrate) {
        _stream->write(' ');
        _stream->print(_median_error);
    }

}

void RMSSensor::setParam(float value, SensorParam p) { 

    switch(p) {
        case SENSOR_PARAM_OFFSET:
            _median = SENSOR_MEDIAN_READING + value;
            break;
        case SENSOR_PARAM_SCALE:
            _max_delta_deviation = value? INTERACTIVE_MAX_SINE_DEVIATION / value : 0.0F;
            break;
        default:
            break;
    }

    Sensor::setParam(value, p); 
};

void SensorManager::print(uint8_t ptr, SensorPrintParam mode) {

    if(!_stream || ptr >= _num_sensors ) return;

    if(mode) _sensors[ptr]->suspend();
    
    _stream->write('(');
    _stream->print(ptr);
    _stream->write(' ');

    if(mode)
        _sensors[ptr]->dump();
    else
        _sensors[ptr]->print();

    _stream->println();

    if(mode) _sensors[ptr]->resume();
}


void SensorManager::register_sensor(Sensor* sensor) {
    if(_num_sensors >= MAX_NUM_SENSORS) return;

    _sensors[_num_sensors] = sensor;
    _sensors[_num_sensors]->set_output(_stream);
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
