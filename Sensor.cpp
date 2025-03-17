#include "Sensor.h"


Sensor::Sensor(int pin, float offset, float scale, int num_samples) {
    pinMode(pin, INPUT);
    _pin = pin;
    _num_samples = num_samples;
    _param[SENSOR_PARAM_OFFSET] = offset;
    _param[SENSOR_PARAM_SCALE] = scale;
    _readings = (int*)malloc(num_samples * sizeof(int));
    reset();
}

void Sensor::sample() {
    int reading = analogRead(_pin);
    
    if(_ready) {
        _reading_sum += reading - *( _readings + _counter );
    }
    else {
        _reading_sum += reading;
    }

    *(_readings + _counter) = reading;
    _counter++;
    _update = true;

    if(_counter >= _num_samples) {
        _ready = true;
        _counter = 0;
    }
}

void Sensor::reset() {
    _counter = 0;
    memset(_readings, 0, _num_samples);
    _reading_sum = 0;
    _ready = false;
    _update = false;
    _reading = 0.0;
}

float Sensor::reading() {
    if(_counter == 0) sample();
    if(_update) {
        float average = (float)_reading_sum / (_ready? _num_samples : _counter);
        _reading = average * _param[SENSOR_PARAM_SCALE]   + _param[SENSOR_PARAM_OFFSET];
        _update = false;
    }

    return _reading;
        
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
