#include "Sensor.h"


Sensor::Sensor(int pin, float offset, float scale, int num_samples) {
    pinMode(pin, INPUT);
    _pin = pin;
    _num_samples = num_samples;
    _offset = offset;
    _scale = scale;
    _readings = (int*)malloc(num_samples * sizeof(int));
    reset();
}

void Sensor::sample() {
    int reading = analogRead(_pin);

    int next_counter = _counter + 1;

    if(_ready) {

        int oldest_reading = *( _readings +  next_counter % _num_samples );

        _average_reading = (float)( reading - oldest_reading ) / _num_samples  +  _average_reading;
    }
    else {
        _average_reading = ((float) reading + _average_reading * _counter ) / next_counter;
    }

    *(_readings + _counter) = reading;
    _counter++;

    if(_counter >= _num_samples) {
        _ready = true;
        _counter = 0;
    }
}

void Sensor::reset() {
    _counter = 0;
    memset(_readings, 0, _num_samples);
    _average_reading = 0.0F;
    _ready = false;
}

float Sensor::reading() {
    if(_counter == 0) sample();
    return  _average_reading * _scale   + _offset;
}

long Sensor::readingR() {
    return round(reading());
}

void Sensor::setSensorParam(float value, SensorParam param) {
    if(param == SENSOR_PARAM_OFFSET)
        _offset = value;
    else 
        _scale = value;
}

float Sensor::getSensorParam(SensorParam param) {
    if(param == SENSOR_PARAM_OFFSET)
        return _offset;
    else 
        return _scale;
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

void SensorManager::saveSensorParams() {
    unsigned int addr = 0;
    EEPROM.put(addr, _num_sensors);
    addr += sizeof(int);
    for(int i=0; i < _num_sensors; i++ ) {
        EEPROM.put(addr, _sensors[i]->getSensorParam(SENSOR_PARAM_SCALE));
        addr += sizeof(float);
        EEPROM.put(addr, _sensors[i]->getSensorParam(SENSOR_PARAM_OFFSET));
        addr += sizeof(float);
    }
}

void SensorManager::loadSensorParams() {
    unsigned int addr = 0;
    
    if(! loadInt(addr) != _num_sensors) {
        saveSensorParams();
        return;
    }

    addr += sizeof(int);

    for(int i=0; i < _num_sensors; i++ ) {
        _sensors[i]->setSensorParam(loadFloat(addr), SENSOR_PARAM_SCALE);
        addr += sizeof(float);
        _sensors[i]->setSensorParam(loadFloat(addr), SENSOR_PARAM_OFFSET);
        addr += sizeof(float);
    }

}

int SensorManager::loadInt(unsigned int addr) {
    int result = 0;

    EEPROM.get(addr, result);

    return result;
}

float SensorManager::loadFloat(unsigned int addr) {
    float result = 0.0F;

    EEPROM.get(addr, result);

    return result;
}
