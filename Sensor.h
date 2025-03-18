#ifndef Sensor_h
#define Sensor_h

#include  "config.h"
#include "Settings.h"

#define DEFAULT_SCALE           1.00
#define DEFAULT_OFFSET          0.00
#define DEFAULT_MEDIAN_READING  512

#define SIGN(X) ((X) > 0) - ((X) < 0)

const int DEFAULT_NUM_SAMPLES = 30;
const int DEFAULT_SAMPLING_RATE = 1;

enum SensorParam {
    SENSOR_PARAM_SCALE,
    SENSOR_PARAM_OFFSET,
    SENSOR_NUMPARAMS
};

/**
 * @brief Sensor class implements a running average of a series of samples from an analog input. 
 * 
 */
class Sensor {
    public:

        Sensor(int pin, float offset = DEFAULT_OFFSET, 
                float scale = DEFAULT_SCALE, 
                int num_samples = DEFAULT_NUM_SAMPLES, 
                int sampling_rate = DEFAULT_SAMPLING_RATE);

        // Takes measurement and accumulate the value. Multiple measurements will be averaged when reading is called.
        void sample();
        
        // Reset all the previously taken measurements 
        void reset();

        // Get the reading of the sensor, converted to measurement units (offset/scale applied).
        // If sample() was not called before calling this function then instant reading will be returned.
        float reading();
        
        //  triggered when the sensor is reset
        virtual void on_reset() {;};

        // triggered on the readings counter overflow
        virtual void on_counter_overflow() {;};

        // accumulate reading in the _reading sum
        virtual void increment_sum(int reading);

        virtual float compute_reading();
        
        // rounded reading
        long readingR() { return round(reading()); };

        // Returns true if necessary number of samples has been taken already
        bool ready() { return _ready; };

        int getNumSamples() { return _num_samples; };

        void setParam(float value, SensorParam p) { _param[p] = value; _update = true;};
        float getParam(SensorParam p) { return _param[p]; };
    
    protected:
        
        float transpose_reading(float value) { return value * _param[SENSOR_PARAM_SCALE]   + _param[SENSOR_PARAM_OFFSET]; };
        
        int _pin;
        bool _ready, _update;

        int _counter;

        int *_readings;

        int _num_samples;
        float _param[SENSOR_NUMPARAMS];

        int _sampling_rate;
        int _sample_counter;

        float _avg_reading;

        long _reading_sum; 

};

/**
 * @brief RMS sensor class is for measuring the amplitude and the period of the periodic signal. 
 *        Amplitude is measured using RMS method. 
 * 
 */
class RMSSensor : public Sensor {
     public:

        using Sensor::Sensor;

        void increment_sum(int reading) override;

        float compute_reading() override;

        void on_reset() override {
            _period = 0;
            _period_counter = _period_sum = 0;
            _period_start = _prev_reading = -1;
        };

        void on_counter_overflow() override;

        // returns avg number of ticks corresponding to the period of the signal
        float get_period() { return _period; };

    private:

        // median reading
        int _median = DEFAULT_MEDIAN_READING;

        float _period;

        // qty of detected periods in the sampling window
        int _period_counter;

        // accumulated period value in ticks.
        int _period_sum;

        // keeps the counter value at the start of the period
        int _period_start;

        // keeps the previous reading
        int _prev_reading;


};

/**
 * @brief SensorManager class orchestrates sensor sampling and parameter handling
 * 
 */
class SensorManager {
    public:
        
        SensorManager( HardwareSerial * dbg, Settings * settings ) {
            _dbg = dbg;
            _settings = settings;
        };

        void registerSensor(Sensor* sensor);

        // Take a sample for all the registered sensors
        void sample();

        Sensor* get(int ptr);

        int getNumSensors() { return _num_sensors; };

        // save sensor params to EEPROM
        void saveParams();
        
        // load sensor params from EEPROM. If sensor params were not saved before, they are initialized in EEPROM
        void loadParams();

    private:
        HardwareSerial * _dbg;

        Settings * _settings;

        Sensor* _sensors[MAX_NUM_SENSORS];
        int _num_sensors = 0;

};

#endif