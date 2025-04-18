#ifndef Sensor_h
#define Sensor_h

#include  "config.h"
#include "utilities.h"
#include "Settings.h"

#define DEFAULT_SCALE           1.00
#define DEFAULT_OFFSET          0.00
#define NOT_DEFINED             -1

const uint8_t SENSOR_NUM_SAMPLES = 20;
const uint8_t SENSOR_SAMPLING_PERIOD = 1;
const uint8_t SENSOR_SAMPLING_PHASE = 0;
const uint8_t SENSOR_NUM_PERIODS = 3;
const int SENSOR_MEDIAN_READING = 512;

const float SENSOR_GRAD_PER_SEC = (float) 360 / TIMER_ONE_SEC;

enum SensorParam {
    SENSOR_PARAM_SCALE,
    SENSOR_PARAM_OFFSET,
    SENSOR_NUMPARAMS
};

enum SensorPrintParam {
    SENSOR_PRINT_PARAM,
    SENSOR_PRINT_DUMP
};

/**
 * @brief Sensor class implements a running average of a series of samples from an analog input. 
 * 
 */
class Sensor {
    public:

        Sensor(int pin, float offset = DEFAULT_OFFSET, 
                        float scale = DEFAULT_SCALE, 
                        uint8_t num_samples = SENSOR_NUM_SAMPLES, 
                        uint8_t sampling_period = SENSOR_SAMPLING_PERIOD,
                        uint8_t sampling_phase = SENSOR_SAMPLING_PHASE);

        // Takes measurement and accumulate the value. Multiple measurements will be averaged when reading is called.
        void sample();
        
        // Initialization 
        void init();

        //  triggered when the sensor is init
        virtual void on_init(){ _active = true; _ready = false; _calibrate = false; };

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

        virtual void dump() {;};

        // print sensor parameters
        virtual void print(); 

        virtual void calibrate() { _calibrate = !_calibrate; };

        void set_output(Print* stream) { _stream = stream; };

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
        
        // if true the sensor is in calibrate mode
        bool _calibrate;

        Print* _stream;

};

class SimpleSensor : public Sensor {
    public:
        SimpleSensor(int pin, float offset = DEFAULT_OFFSET, 
            float scale = DEFAULT_SCALE, 
            uint8_t num_samples = SENSOR_NUM_SAMPLES, 
            uint8_t sampling_period = SENSOR_SAMPLING_PERIOD,
            uint8_t sampling_phase = SENSOR_SAMPLING_PHASE);

        void on_init() override;

        void reset() override;

        void increment_sum(int reading) override;

        void compute_reading() override;

        void on_counter_overflow() override { _ready = true;};

        void dump() override;

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
            uint8_t num_samples = SENSOR_NUM_SAMPLES, 
            uint8_t sampling_period = SENSOR_SAMPLING_PERIOD,
            uint8_t sampling_phase = SENSOR_SAMPLING_PHASE,
            uint16_t num_periods = SENSOR_NUM_PERIODS); 

        void increment_sum(int reading) override;

        void compute_reading() override;

        void reset() override;

        void on_init() override;

        void on_counter_overflow() override;

        void dump() override;

        void print() override;

        // returns avg number of ticks corresponding to the period of the signal
        float get_period() { return _avg_period; };

        // returns the frequency of the signal in Hz
        float get_frequency() { return _avg_frequency; };

        int get_median_error() { return _median_error ; };

        bool bad_sine() { bool lbs = _last_bad_sine; _last_bad_sine = _bad_sine; return _bad_sine && lbs; };

    protected:
        float transpose_reading(float value) override { return value * _param[SENSOR_PARAM_SCALE]; };

        void setParam(float value, SensorParam p) override;

    private:

        // median reading
        int _median;

        // variables needed for calibration
        int _running_median_error; 
        int _median_error;

        // average period computed 
        volatile float _avg_period;

        // average frequency computed
        volatile float _avg_frequency;

        // if true bad sine detected
        volatile bool _bad_sine;
        volatile bool _last_bad_sine;
        
        // last observed amplitude of the signal in ADC units
        int _last_amplitude;

        float _expected_delta;

        int _running_max_delta;

        float _max_delta_deviation;
        
        // counter of ticks elapsed since the start of the period
        int _period_tick_counter;
        
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
        
        SensorManager( Settings * settings , Print * stream = nullptr) {
            _stream = stream;
            _active = true;
            _settings = settings;
        };

        void register_sensor(Sensor* sensor);

        // Take a sample for all the registered sensors
        void sample();

        Sensor* get(uint8_t ptr) { return _sensors[ptr]; };

        void print(uint8_t ptr, SensorPrintParam mode = SENSOR_PRINT_PARAM );

        uint8_t get_num_sensors() { return _num_sensors; };

        // save sensor params to EEPROM
        void saveParams();
        
        // load sensor params from EEPROM. If sensor params were not saved before, they are initialized in EEPROM
        void loadParams();

        void suspend() { _active = false; };
        void resume() { _active = true; };

    private:
        Print * _stream;

        Settings * _settings;

        Sensor* _sensors[MAX_NUM_SENSORS];
        uint8_t _num_sensors = 0;

        bool _active;

};

#endif