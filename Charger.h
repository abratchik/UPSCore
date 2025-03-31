#ifndef Charger_h
#define Charger_h

#include <Print.h>

#include "Settings.h"
#include "Sensor.h"

// Maximum charging pulse width = 0.5
#define MAXCOUT 512

#define sgn(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : 0))

#define DEFAULT_CHARGER_PWM_OUT     10     // PWM out


enum ChargingStatus {
    CHARGING_NOT_STARTED,
    CHARGING_BY_CC,
    CHARGING_BY_CV, 
    CHARGING_COMPLETE,              // like CV but with standby voltage
    CHARGING_C_SENSOR_NOT_SET,
    CHARGING_C_SENSOR_NOT_READY,
    CHARGING_V_SENSOR_NOT_SET,
    CHARGING_V_SENSOR_NOT_READY,
    CHARGING_TARGET_NOT_SET,
    CHARGING_REPLACE_BATTERY,
    CHARGING_BATTERY_DEAD
};

enum ChargerPIDParam {
    CHARGING_KP,
    CHARGING_KI,
    CHARGING_KD,
    CHARGING_TEST,
    CHARGING_NUMPARAM
};

/**
 * @brief Charger is the class responsible for managing the battery charging. This class uses the battery 
 *        and current sensor readings to manipulate the PWM output on the pin 10 of Arduino Nano.  
 *        Frequency of the PWM output is defined by Timer 1 registers, which are set in the UPSCore.ino setup() function.
 *        The charging circuit through the battery is defined by the PWM signal. Regulation is done with help of PID regulator
 *        where the battery voltage and current are used as inputs, depending on the phase of the charge (CC, CV or backup).
 */
class Charger {
    public:

        Charger(Settings * settings, Sensor* current_sensor, Sensor* voltage_sensor, Print* dbg = nullptr);   

        void set_current_sensor(Sensor* current_sensor) {_current_sensor = current_sensor;};
        void set_voltage_sensor(Sensor* voltage_sensor) {_voltage_sensor = voltage_sensor;};

        // Set the target charging current.
        void set_current( float charging_current );

        float get_current() { return _charging_current; };

        // Set the target charging voltage.
        void set_voltage(float charging_voltage) {_charging_voltage = charging_voltage;};

        float get_voltage() { return _charging_voltage; };    

        void set_min_battery_voltage(float min_battery_voltage) {_min_battery_voltage = min_battery_voltage; }; 
        void set_cutoff_current(float cutoff_current) { _cutoff_current = cutoff_current; };

        float get_last_deviation() { return _last_deviation; };
        int get_output() { return _cout_regv; };

        int get_elapsed_ticks() { return _elapsed_ticks; };

        // Increase or decrease cout_regv depending on the sensor reading
        // Current and voltage sensors must be set before calling
        // @param ticks current time in ticks 
        void regulate( unsigned long ticks );                                   
                                                             
        void start(float charging_current, float charging_voltage, unsigned long ticks);

        void stop();

        bool is_charging() { return _charging; };

        uint16_t get_mode() { return _charging_mode; };

        void set_mode( uint16_t charging_mode ) { _charging_mode = charging_mode; };

        void setParam(float value, ChargerPIDParam param) { k[param] = value; };
        float getParam(ChargerPIDParam param) { return k[param]; };

        void loadParams();

        void saveParams();

    private:
        Print* _dbg;

        Settings * _settings;

        // charging current value (to be compared with sensor)
        float _charging_current;      

        // charging voltage value (to be compared with sensor)
        float _charging_voltage;   

        // Integrated error
        float _deviation_sum;  

        // Last deviation for calculating derivative
        float _last_deviation;     

        // the ticks on the latest regulate() or start() call
        unsigned long _last_ticks;  
        int _elapsed_ticks;   

        // PID params
        float k[CHARGING_NUMPARAM];   
          
        uint16_t _charging_mode;     
        
        // regulator value (0-1023)  
        int _cout_regv;                     

        float _min_battery_voltage;

        float _cutoff_current;

        Sensor* _current_sensor = NULL;
        Sensor* _voltage_sensor = NULL;

        bool _charging;

        void set_charging(bool charging) {
            _charging = charging;
            _deviation_sum = _last_deviation = 0;
            _cout_regv = 0;
            pwmSet10(0);  
        };

        // analogWrite replacement for FastPWM 10-bit mode on pin 10
        void pwmSet10(int value);

};
     

#endif