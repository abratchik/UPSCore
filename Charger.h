#ifndef Charger_h
#define Charger_h

#include "Sensor.h"

#define MAXCOUT 254
#define sgn(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : 0))

#define DEFAULT_CHARGER_OUT         4      // Charger ON/OFF out
#define DEFAULT_CHARGER_PWM_OUT     10     // PWM out


enum ChargingStatus {
    CHARGING_BY_CC,
    CHARGING_BY_CV, 
    CHARGING_C_SENSOR_NOT_SET,
    CHARGING_C_SENSOR_NOT_READY,
    CHARGING_V_SENSOR_NOT_SET,
    CHARGING_V_SENSOR_NOT_READY,
    CHARGING_NOT_STARTED,
    CHARGING_MAX_HIT,
    CHARGING_MIN_HIT,
    CHARGING_TARGET_NOT_SET,
    CHARGING_BATTERY_DEAD,
    CHARGING_INIT,
    CHARGING_STARTED,
    CHARGING_COMPLETE,
    CHARGING_FAILED 
};

class Charger {
    public:

        //Only 16-bit timer can be used
        Charger(Sensor* current_sensor, Sensor* voltage_sensor, 
                int cout_pin = DEFAULT_CHARGER_PWM_OUT, 
                int charging_pin = DEFAULT_CHARGER_OUT);   

        void set_current_sensor(Sensor* current_sensor) {_current_sensor = current_sensor;};
        void set_voltage_sensor(Sensor* voltage_sensor) {_voltage_sensor = voltage_sensor;};

        // Set the target charging current.
        void set_current(float charging_current);

        float get_current() {return _charging_current;};

        // Set the target charging voltage.
        void set_voltage(float charging_voltage) {_charging_voltage = charging_voltage;};

        float get_voltage() { return _charging_voltage; };    

        void set_min_charge_voltage(float min_charge_voltage) {_min_charge_voltage = min_charge_voltage; }; 

        float get_min_charge_voltage() { return _min_charge_voltage; };
        
        void set_cutoff_current(float cutoff_current) { _cutoff_current = cutoff_current; };

        float get_cutoff_current() { return _cutoff_current; };

        // Increase or decrease cout_regv depending on the sensor reading
        // Current and voltage sensors must be set before calling
        void regulate();                                   
                                                             
        void start(float charging_current, float charging_voltage);

        void stop();

        bool is_charging() { return _charging; };

        int get_mode() { return _charging_mode; };

        void set_mode( int charging_mode ) { _charging_mode = charging_mode; };

    private:

        // PWM signal managing the charging current and the on/off charging signal
        int _cout_pin, _charging_pin;   

        // charging current value (to be compared with sensor)
        float _charging_current = 0;      

        // charging voltage value (to be compared with sensor)
        float _charging_voltage = 0;   
          
        ChargingStatus _charging_mode;     
        
        // regulator value (0-1023)  
        int _cout_regv = 0;                     

        float _min_charge_voltage = 0;

        float _cutoff_current = 0;

        Sensor* _current_sensor = NULL;
        Sensor* _voltage_sensor = NULL;

        bool _charging = false;

        void set_charging(bool charging) {
            _charging = charging;
            analogWrite(_charging_pin, ( charging? HIGH : LOW ) );
        };


};
     

#endif