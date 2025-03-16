#ifndef Sensor_h
#define Sensor_h

#include  "config.h"
#include "Settings.h"

#define DEFAULT_SCALE 1.00
#define DEFAULT_OFFSET 0.00
#define DEFAULT_NUM_SAMPLES 30

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

        Sensor(int pin, float offset = DEFAULT_OFFSET, float scale = DEFAULT_SCALE, int num_samples = DEFAULT_NUM_SAMPLES);

        // Takes measurement and accumulate the value. Multiple measurements will be averaged when reading is called.
        void sample();
        
        // Reset all the previously taken measurements 
        void reset();
        
        // Get the reading of the sensor, converted to measurement units (offset/scale applied).
        // If sample() was not called before calling this function then instant reading will be returned.
        float reading();
        
        // rounded reading
        long readingR() { return round(reading()); };

        // Returns true if necessary number of samples has been taken already
        bool ready() { return _ready; };

        void setNumSamples( int num_samples = DEFAULT_NUM_SAMPLES ) { _num_samples = num_samples; };
        int getNumSamples() { return _num_samples; };

        void setParam(float value, SensorParam p) { _param[p] = value; };
        float getParam(SensorParam p) { return _param[p]; };
    
    protected:
        int _pin;
        bool _ready;

        int _counter = 0;

        int *_readings;

        int _num_samples = DEFAULT_NUM_SAMPLES;
        float _param[SENSOR_NUMPARAMS];

    private:

        float _average_reading = 0; 

};

// class RMSSensor : public Sensor {
//     int get_pin() {return _pin;};
// };

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