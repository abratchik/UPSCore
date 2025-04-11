#include "Sensor.h"


Sensor::Sensor(int pin, float offset, float scale, uint8_t num_samples, uint8_t sampling_period, uint8_t sampling_phase, Print* stream) {
    pinMode(pin, INPUT);
    _pin = pin;
    _num_samples = num_samples;
    _param[SENSOR_PARAM_OFFSET] = offset;
    _param[SENSOR_PARAM_SCALE] = scale;
    _sampling_period = sampling_period;
    _sampling_phase = sampling_phase;

    _stream = stream;

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
    _active = true;
    _counter = 0;
    _reading_sum = 0L;
    _ready = false;
    _avg_reading = 0.0F;
    _sample_counter = _sampling_phase % _sampling_period;
    _last_reading = NOT_DEFINED; // this is to indicate that the sensor has not been sampled yet
}

SimpleSensor::SimpleSensor(int pin, float offset,  float scale, uint8_t num_samples, uint8_t sampling_period , uint8_t sampling_phase, 
                           Print* stream) :
    Sensor(pin, offset, scale, num_samples, sampling_period, sampling_phase, stream) {
    init();
} 

void SimpleSensor::reset() {
    Sensor::reset();
    memset(_readings, 0x0, _num_samples * sizeof(int));
}

void SimpleSensor::on_init() {
    _readings = (int*) calloc(_num_samples, sizeof(int)); 
}

void SimpleSensor::compute_reading() {
    if(!_ready ) return;
    float total = _reading_sum;
    _avg_reading  = transpose_reading( total / _num_samples  );
}

void SimpleSensor::dump_readings() {
    if(!_stream) return;
    suspend();

    _stream->write('(');
    for(int i=0; i < _num_samples; i++) {
        if(i) _stream->write(',');
        _stream->print(*(_readings+i)); 
    }
    _stream->println();

    resume();
}

RMSSensor::RMSSensor(int pin, float offset,  float scale, 
                     uint8_t num_samples, uint8_t sampling_period , uint8_t sampling_phase, uint16_t num_periods, Print* stream) :
    Sensor(pin, offset, scale, num_samples, sampling_period, sampling_phase, stream) {
    _num_periods = num_periods;    
    init();
} 

void RMSSensor::reset() {
    Sensor::reset();

    _avg_period = 0.0F;
    _period_index = 0;
    _period_counter = 0;
    _period_sum = 0;
    _period_start = NOT_DEFINED;
    _running_sum  = 0L;

    memset(_sq_deltas, 0x0, _num_periods * sizeof(long));
    memset(_periods, 0x0, _num_periods * sizeof(int));
}

void RMSSensor::on_init() {
    _sq_deltas = (long*) calloc( _num_periods , sizeof(long)); 
    _periods = (int*) calloc( _num_periods , sizeof(int)); 

    _median = DEFAULT_MEDIAN_READING + _param[SENSOR_PARAM_OFFSET];
}

void RMSSensor::compute_reading() {
    if(!_ready ) return;
    float total_delta_sq = _reading_sum;
    int timeframe = _period_sum * _sampling_period;
    _avg_reading = _period_sum? transpose_reading(sqrtf( total_delta_sq / timeframe )): 0;  
    _avg_period = (float) timeframe / _num_periods ;
}

// This method updates the running sum of squared deltas and tracks the period of the signal.
// It calculates the difference (delta) between the current reading and the median, 
// and updates the running sum of squared deltas. Additionally, it detects when the signal 
// crosses the median from negative to positive, marking the start and end of a period.
void RMSSensor::increment_sum(int reading) {

    int delta = reading - _median;

    if(_period_start != NOT_DEFINED) {
        _running_sum += square(delta);
    }

    // reading is crossing the median from negative to positive
    if( delta > 0 && _last_reading != NOT_DEFINED && (_last_reading <= _median ) ) {

        uint16_t period_ptr = _counter;
        
        // end of the period reached
        if(_period_start != NOT_DEFINED ) {
            
            int period = 0;

            if(period_ptr >= _period_start) 
                period = period_ptr - _period_start;
            else
                period = _counter + _num_samples - _period_start;

            long old_reading = *(_sq_deltas + _period_index);
            int old_period = *(_periods + _period_index);

            _reading_sum += _running_sum - old_reading;
            _period_sum += period - old_period;

            *(_sq_deltas + _period_index) = _running_sum;
            *(_periods + _period_index) = period;
            
            _running_sum  = 0L;
            _period_index++;
            _period_counter++;

            if(_period_index >= _num_periods) {
                _period_index = 0;
                // once the required number of periods received, the sensor is ready
                _ready = true;
            }
        }
        
        _period_start = period_ptr;
    }

}

void RMSSensor::on_counter_overflow() {
    
    // sensor is also ready once the end of sampling window is reached
    _ready = true;

    // if no periods were detected during the _num_samples, there is no signal or
    // the frequency is too low or too high to measure for the given sampling window (_num_samples * _sampling_period )
    if(!_period_counter || _period_counter > (int)(_num_samples >> 1) ) {
        _reading_sum = 0L;
        _period_sum = 0;
        _avg_reading = 0.0F;
        _avg_period = 0.0F;
        _reading_sum = 0L;
    }

    _period_counter = 0;

}

void RMSSensor::dump_readings() {
    if(!_stream) return;
    suspend();

    _stream->write('(');
    for(int i=0; i<_num_periods; i++) {
        if(i) _stream->write(';');
        _stream->print(*(_sq_deltas+i)); 
        _stream->write(',');
        _stream->print(*(_periods+i)); 
    }
    _stream->println();

    resume();
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
