#ifndef Sensor_h
#define Sensor_h

#include  "config.h"
#include "Settings.h"

#define DEFAULT_SCALE           1.00
#define DEFAULT_OFFSET          0.00

const uint8_t DEFAULT_NUM_SAMPLES = 20;
const uint8_t DEFAULT_SAMPLING_PERIOD = 1;
const uint8_t DEFAULT_SAMPLING_PHASE = 0;
const uint8_t DEFAULT_NUM_FRAMES = 3;
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
                uint8_t sampling_phase = DEFAULT_SAMPLING_PHASE);

        // Takes measurement and accumulate the value. Multiple measurements will be averaged when reading is called.
        void sample();
        
        // Initialization 
        void init();


        float reading(){ return _avg_reading; };
        
        //  triggered when the sensor is init
        virtual void on_init() { ; };

        // triggered on the readings counter overflow
        virtual void on_counter_overflow() { _ready = true;};

        // accumulate reading in the _reading sum
        virtual void increment_sum(int reading);

        // Compute the reading of the sensor, converted to measurement units (offset/scale applied).
        virtual void compute_reading();
        
        // rounded reading
        long readingR() { return round(reading()); };

        // Returns true if necessary number of samples has been taken already
        bool ready() { return _ready; };

        uint8_t get_num_samples() { return _num_samples; };

        int get_last_reading() { return _last_reading; };

        long get_reading_sum() { return _reading_sum; };

        virtual int get_median() { return 0; };

        int* get_readings() { return _readings; };

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

        // pointer to the readings storage
        int *_readings;

};

/**
 * @brief RMS sensor class is for measuring the effective amplitude and the period of the periodic signal. 
 *        Amplitude is measured using the True RMS method. Period is 
 * 
 */
class RMSSensor : public Sensor {
     public:

        RMSSensor(int pin, float offset = DEFAULT_OFFSET, 
            float scale = DEFAULT_SCALE, 
            uint8_t num_samples = DEFAULT_NUM_SAMPLES, 
            uint8_t sampling_period = DEFAULT_SAMPLING_PERIOD,
            uint8_t sampling_phase = DEFAULT_SAMPLING_PHASE,
            uint8_t num_frames = DEFAULT_NUM_FRAMES
        ); 

        void increment_sum(int reading) override;

        void compute_reading() override;

        void on_init() override {
            _median = DEFAULT_MEDIAN_READING + _param[SENSOR_PARAM_OFFSET];
            _running_period_sum = 0;
            _period = 0;
            _avg_period = 0.0F;
            _period_counter = 0;
            _period_sum = 0;
            _period_start = -1;
            _running_sum  = 0L;
            _frame_counter = 0;
            _running_sum_total = 0;
        };

        void on_counter_overflow() override;

        // returns avg number of ticks corresponding to the period of the signal
        float get_period() { return _avg_period; };

        // returns the frequency of the signal in Hz
        float get_frequency() { return _avg_period > 0.0? round( (float) TIMER_ONE_SEC / _avg_period ) : 0 ; };

        int get_median() override { return _median; };

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

        // average period computed 
        volatile float _avg_period;
        
        // latest detected value of the period in ticks
        uint16_t _period;

        // qty of detected periods 
        uint16_t _period_counter;

        // accumulated period values in ticks.
        uint16_t _period_sum;

        uint8_t _frame_counter;
        uint8_t _num_frames;

        // keeps the counter value at the start of the period
        int _period_start;

        // accumulated sum of squares within the period
        long _running_sum;

        long _running_sum_total;

        uint16_t _running_period_sum;


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