#ifndef Sensor_h
#define Sensor_h

#include  "config.h"

#define DEFAULT_SCALE 1.00
#define DEFAULT_OFFSET 0.00
#define DEFAULT_NUM_SAMPLES 30

class Sensor {
    public:

        Sensor(int pin, float offset = DEFAULT_OFFSET, float scale = DEFAULT_SCALE, int num_samples = DEFAULT_NUM_SAMPLES);

        // Take measurement and accumulate the value. Multiple measurements will be averaged when reading is called.
        void sample();
        
        // Reset all the previously taken measurements 
        void reset();
        
        // Get the reading of the sensor, converted to measurement units (offset/scale applied).
        // If sample() was not called before calling this function then instant reading will be returned.
        float reading();
        
        // rounded reading
        long readingR();

        // Returns true if necessary number of samples has been taken already
        bool ready() { return _ready; };
        
        void setOffset(float offset = DEFAULT_OFFSET ) { _offset = offset; };
        float getOffset() { return _offset; };

        void setScale(float scale = DEFAULT_SCALE ) { _scale = scale; };
        float getScale() { return _scale; };

        void setNumSamples( int num_samples = DEFAULT_NUM_SAMPLES ) { _num_samples = num_samples; };
        int getNumSamples() { return _num_samples; };

    private:
        int _pin;
        bool _ready;

        float _average_reading = 0.0; 
        int _counter = 0;

        int *_readings;

        int _num_samples = DEFAULT_NUM_SAMPLES;
        float _offset = DEFAULT_OFFSET;
        float _scale = DEFAULT_SCALE;

};

#endif