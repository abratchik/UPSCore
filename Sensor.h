#ifndef Sensor_h
#define Sensor_h

#include  "config.h"
#include "Settings.h"

#define DEFAULT_SCALE           1.00
#define DEFAULT_OFFSET          0.00
#define NOT_DEFINED             -1

const uint8_t DEFAULT_NUM_SAMPLES = 20;
const uint8_t DEFAULT_SAMPLING_PERIOD = 1;
const uint8_t DEFAULT_SAMPLING_PHASE = 0;
const uint8_t DEFAULT_NUM_PERIODS = 3;
const int DEFAULT_MEDIAN_READING = 512;

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
                        uint8_t num_samples = DEFAULT_NUM_SAMPLES, 
                        uint8_t sampling_period = DEFAULT_SAMPLING_PERIOD,
                        uint8_t sampling_phase = DEFAULT_SAMPLING_PHASE,
                        Print* stream = nullptr);

        // Takes measurement and accumulate the value. Multiple measurements will be averaged when reading is called.
        void sample();
        
        // Initialization 
        void init();

        //  triggered when the sensor is init
        virtual void on_init(){ _active = true; _ready = false; };

        virtual void reset();

        float reading(){ return _avg_reading; };

        // triggered on the readings counter overflow
        virtual void on_counter_overflow(){;};

        // accumulate reading in the _reading sum
        virtual void increment_sum(int reading){;};

        // Compute the reading of the sensor, converted to measurement units (offset/scale applied).
        virtual void compute_reading(){;};
        
        // rounded reading
        long readingR() { return round(reading()); };

        // Returns true if necessary number of samples has been taken already
        bool ready() { return _ready; };

        int get_last_reading() { return _last_reading; };

        long get_reading_sum() { return _reading_sum; };

        virtual int get_median() { return 0; };

        virtual void dump_readings() {;};

        virtual void setParam(float value, SensorParam p) { _param[p] = value; compute_reading();};
        float getParam(SensorParam p) { return _param[p]; };

        void suspend() { _active = false; };
        void resume() { _active = true; };
    
    protected:
        
        virtual float transpose_reading(float value) { return value * _param[SENSOR_PARAM_SCALE]   + _param[SENSOR_PARAM_OFFSET]; };
        
        // ADC input pin number
        int _pin;

        // sensor is ready for the reading computation
        bool _ready;
        
        // counter of readings 
        uint8_t _counter;
        
        // qty of readings
        uint8_t _num_samples;

        // transpose factors (used for calculation  of the sensor reading)
        float _param[SENSOR_NUMPARAMS];
        
        // number of ticks between the samples
        uint8_t _sampling_period;
        // offset in ticks for the first reading
        uint8_t _sampling_phase;
        // counter of the sample() calls
        uint8_t _sample_counter;
        
        // calculated sensor reading
        volatile float _avg_reading;
        
        // accumulated value for readings used for the sensor reading calculation
        long _reading_sum; 

        // keeps the last ADC reading
        int _last_reading;     
        
        // if true the sample() call is void
        bool _active;

        Print* _stream;

};

class SimpleSensor : public Sensor {
    public:
        SimpleSensor(int pin, float offset = DEFAULT_OFFSET, 
            float scale = DEFAULT_SCALE, 
            uint8_t num_samples = DEFAULT_NUM_SAMPLES, 
            uint8_t sampling_period = DEFAULT_SAMPLING_PERIOD,
            uint8_t sampling_phase = DEFAULT_SAMPLING_PHASE, 
            Print* stream = nullptr);

        void on_init() override;

        void reset() override;

        void increment_sum(int reading) override;

        void compute_reading() override;

        void on_counter_overflow() override { _ready = true;};

        void dump_readings() override;

    private:
        // pointer to the readings storage
        int *_readings;
};

/**
 * @brief RMS sensor class is for measuring the effective amplitude and the period of the periodic signal. 
 *        Amplitude is measured using the True RMS method. 
 * 
 */
class RMSSensor : public Sensor {
     public:

        RMSSensor(int pin, float offset = DEFAULT_OFFSET, 
            float scale = DEFAULT_SCALE, 
            uint8_t num_samples = DEFAULT_NUM_SAMPLES, 
            uint8_t sampling_period = DEFAULT_SAMPLING_PERIOD,
            uint8_t sampling_phase = DEFAULT_SAMPLING_PHASE,
            uint16_t num_periods = DEFAULT_NUM_PERIODS,
            Print* stream = nullptr); 

        void increment_sum(int reading) override;

        void compute_reading() override;

        void reset() override;

        void on_init() override;

        void on_counter_overflow() override;

        void dump_readings() override;

        // returns avg number of ticks corresponding to the period of the signal
        float get_period() { return _avg_period; };

        // returns the frequency of the signal in Hz
        float get_frequency() { return _avg_period > 0.0? round( (float) TIMER_ONE_SEC / _avg_period ) : 0 ; };

        int get_median() override { return _median_error ; };

    protected:
        float transpose_reading(float value) override { return value * _param[SENSOR_PARAM_SCALE]; };

        void setParam(float value, SensorParam p) override { 
            if(p == SENSOR_PARAM_OFFSET) {
                _median = DEFAULT_MEDIAN_READING + value;
            }
            Sensor::setParam(value, p); 
        };

    private:


        // median reading
        int _median;
        int _running_median_error;
        int _median_error;

        // average period computed 
        volatile float _avg_period;

        // qty of detected periods 
        int _period_counter;

        // pointer at the last detected period
        int _period_index;

        // max number of periods to analyze
        uint16_t _num_periods;

        // accumulated periods in ticks.
        int _period_sum;

        // keeps the counter value at the start of the period
        int _period_start;

        // accumulated sum of squares within the period
        long _running_sum;

        long *_sq_deltas;
        int *_periods;


};

/**
 * @brief SensorManager class orchestrates sensor sampling and parameter handling
 * 
 */
class SensorManager {
    public:
        
        SensorManager( Settings * settings , HardwareSerial * dbg = nullptr) {
            _dbg = dbg;
            _active = true;
            _settings = settings;
        };

        void register_sensor(Sensor* sensor);

        // Take a sample for all the registered sensors
        void sample();

        Sensor* get(uint8_t ptr) { return _sensors[ptr]; };

        uint8_t get_num_sensors() { return _num_sensors; };

        // save sensor params to EEPROM
        void saveParams();
        
        // load sensor params from EEPROM. If sensor params were not saved before, they are initialized in EEPROM
        void loadParams();

        void suspend() { _active = false; };
        void resume() { _active = true; };

    private:
        HardwareSerial * _dbg;

        Settings * _settings;

        Sensor* _sensors[MAX_NUM_SENSORS];
        uint8_t _num_sensors = 0;

        bool _active;

};

#endif