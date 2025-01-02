#ifndef Interactive_h
#define Interactive_h

#include "config.h"

#include "SimpleTimer.h"
#include "Sensor.h"
#include "Charger.h"


const static float INTERACTIVE_V_BAT_DELTA = INTERACTIVE_MAX_V_BAT - INTERACTIVE_MIN_V_BAT;

enum InteractiveStatusFlags {
    BEEPER_IS_ACTIVE,           // Beeper activated
    SHUTDOWN_ACTIVE,            // UPS is in shutdown mode
    SELF_TEST,                  // UPS is in self-test state
    LINE_INTERACTIVE,           // 1 if line interactive else on-line 
    UPS_FAULT,                  // UPS fault (wrong output)
    REGULATE_FLAG,              // boost or back mode is active
    BATTERY_LOW,
    UTILITY_FAIL,               // input voltage is not within the limits
    BATTERY_DEAD_FLAG,          // Battery dead flag
    UNUSUAL_FLAG,               // 
    INPUT_RELAY_FLAG ,          // Input relay state
    OUTPUT_RELAY_FLAG,          // Output relay state
    OVERLOAD_FLAG               // UPS is in the overload protection mode, restart required
};

enum RegulateMode {
    REGULATE_NONE,
    REGULATE_UP,
    REGULATE_DOWN
};

enum RegulateStatus {
    REGULATE_STATUS_NONE,
    REGULATE_STATUS_SUCCESS,
    REGULATE_STATUS_FAIL,
    REGULATE_STATUS_ERROR
};


class Interactive {
    public:
        Interactive(Sensor *vac_in, Sensor *vac_out, Sensor *ac_out, Sensor *v_bat);

        // AC regulate function (to be called in the loop)
        RegulateStatus regulate();

        void setNominalVACInput(float nominal_vac_input = INTERACTIVE_DEFAULT_INPUT_VOLTAGE,
                                float deviation = INTERACTIVE_INPUT_VOLTAGE_DEVIATION,
                                float hysteresis = INTERACTIVE_INPUT_VOLTAGE_HYSTERESIS ) {
                                    _nominal_vac_input = nominal_vac_input;
                                    _deviation = deviation;
                                    _hysteresis = hysteresis;
                                };

        bool isBatteryMode() { return _batteryMode; };

        uint16_t getStatus() { return _status; };

        float getLastFaultInputVoltage() { return _last_fault_input_voltage; };

        void toggleBeeper() { _status ^= (uint16_t)1 << BEEPER_IS_ACTIVE;}

        float getBatteryLevel() { return _battery_level; };

        void startInverter();

        void stopInverter();

        void connectOutput();

        void disconnectOutput();

        // connect to the mains
        void connectInput();

        // disconnect from the mains 
        void disconnectInput();

        void startSelfTest();

        void stopSelfTest();

        void adjustOutput(RegulateMode mode = REGULATE_NONE);
    
    private:
        Sensor *_vac_in, *_vac_out, *_ac_out, *_v_bat;

        SimpleTimer *_beeper_timer;

        float _nominal_vac_input = INTERACTIVE_DEFAULT_INPUT_VOLTAGE;
        float _deviation = INTERACTIVE_INPUT_VOLTAGE_DEVIATION;
        float _hysteresis = INTERACTIVE_INPUT_VOLTAGE_HYSTERESIS;

        float _last_fault_input_voltage = 0;

        uint16_t _status = 0;

        bool _batteryMode = false;

        float _battery_level; 

};

#endif