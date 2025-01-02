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

