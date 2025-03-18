#include "Sensor.h"


Sensor::Sensor(int pin, float offset, float scale, int num_samples, int sampling_rate) {
    pinMode(pin, INPUT);
    _pin = pin;
    _num_samples = num_samples;
    _param[SENSOR_PARAM_OFFSET] = offset;
    _param[SENSOR_PARAM_SCALE] = scale;
    _sampling_rate = sampling_rate;
    _readings = (int*)malloc(num_samples * sizeof(int));
    reset();
}

void Sensor::sample() {

    _sample_counter = ( _sample_counter++ ) % _sampling_rate ;
    
    if(! _sample_counter ) return;

    int reading = analogRead(_pin);
    
    increment_sum(reading);

    *(_readings + _counter) = reading;
    _counter++;
    _update = true;

    if(_counter >= _num_samples) {
        _ready = true;
        _counter = 0;
        on_counter_overflow();
    }
}

void Sensor::increment_sum(int reading) {
    if(_ready) {
        _reading_sum += reading - *( _readings + _counter );
    }
    else {
        _reading_sum += reading;
    }
}

void Sensor::reset() {
    _counter = 0;
    memset(_readings, 0, _num_samples);
    _reading_sum = 0;
    _ready = false;
    _update = false;
    _avg_reading = 0.0;
    _sample_counter = 0;

    on_reset();
}

float Sensor::compute_reading() {
    return transpose_reading( (float)_reading_sum / (_ready? _num_samples : _counter ) );
}

float RMSSensor::compute_reading() {
    return transpose_reading(sqrtf( (float)_reading_sum / ( _ready? _num_samples : _counter )));  
}

float Sensor::reading() {
    if(_counter == 0) sample();

    if(_update) {
        _avg_reading = compute_reading();
        _update = false;
    }

    return _avg_reading;
        
}

void RMSSensor::increment_sum(int reading) {

    int old_reading = *( _readings + _counter );

    int delta = reading - _median;

    if(_ready) {
        _reading_sum += ( reading + old_reading - 2 * _median ) * ( reading - old_reading );

    }
    else {
        _reading_sum += square(delta);
    }

    // check if the reading is crossing the median from negative to positive
    if( delta > 0 && _prev_reading > 0 && (_prev_reading - _median) <= 0 ) {
        if(_period_start >= 0 ) {
            _period_counter++;
            _period_sum += _counter - _period_start;
        }
        _period_start = _counter;
    }

    _prev_reading = reading;
}

void RMSSensor::on_counter_overflow() {
    _period_start = -1;

    // if no periods were counted, the frequency is too low to measure within the sampling window
    if(!_period_counter) {
        _period = 0;
        _period_sum = 0;
        return; 
    }

    _period = (float)_period_sum * _sampling_rate / _period_counter ;

    _period_sum = _period_counter = 0;
}

void SensorManager::registerSensor(Sensor* sensor) {
    if(_num_sensors >= MAX_NUM_SENSORS) return;

    _sensors[_num_sensors] = sensor;
    _num_sensors++;

    // _dbg->print(_num_sensors);_dbg->write(' ');
    // _dbg->println(" sensor registered");
}

void SensorManager::sample() {
    for(int i=0; i < _num_sensors; i++)
        _sensors[i]->sample();
}

Sensor* SensorManager::get(int ptr) {
    if(ptr >= _num_sensors) return nullptr;

    return _sensors[ptr];
}

void SensorManager::saveParams() {
    long addr = _settings->getAddr(SETTINGS_SENSORS);

    EEPROM.put(addr, _num_sensors);
    addr += sizeof(int);

    for(int i=0; i < _num_sensors; i++ ) {
        for( int p = 0; p < SENSOR_NUMPARAMS; p++ ) {
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
