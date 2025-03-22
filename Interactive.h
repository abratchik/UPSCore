#ifndef Interactive_h
#define Interactive_h

#include "config.h"

#include "SimpleTimer.h"
#include "Sensor.h"
#include "Charger.h"

// time when the UPS stays on battery after failure even if the input is back to normal
// needed to protect the invertor from frequent on/off cycles when the input voltage is oscillating
// around the lowest acceptable limit.
const int INVERTER_GRACE_PERIOD = TIMER_ONE_SEC * 2;

enum InteractiveStatusFlags {
    BEEPER_IS_ACTIVE,           // Beeper activated
    SHUTDOWN_ACTIVE,            // UPS is in shutdown mode
    SELF_TEST,                  // UPS is in self-test state
    LINE_INTERACTIVE,           // 1 if line interactive else on-line 
    UPS_FAULT,                  // UPS fault (wrong output)
    REGULATED,                  // boost or back mode is active
    BATTERY_LOW,
    UTILITY_FAIL,               // input voltage is not within the limits
    BATTERY_DEAD,               // Battery dead flag
    UNUSUAL_STATE,              // 
    INPUT_CONNECTED ,           // Input relay state
    OUTPUT_CONNECTED,           // Output relay state
    OVERLOAD                    // UPS is in the overload protection mode, restart required
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
    REGULATE_STATUS_ERROR,
    REGULATE_STATUS_SHUTDOWN
};


class Interactive {
    public:
        Interactive(RMSSensor *vac_in, RMSSensor *vac_out, Sensor *ac_out, Sensor *v_bat);

        // AC regulate function (to be called in the loop)
        RegulateStatus regulate(unsigned long ticks);

        void setNominalVACInput(float nominal_vac_input = INTERACTIVE_DEFAULT_INPUT_VOLTAGE,
                                float deviation = INTERACTIVE_INPUT_VOLTAGE_DEVIATION,
                                float hysteresis = INTERACTIVE_INPUT_VOLTAGE_HYSTERESIS ) {
                                    _nominal_vac_input = nominal_vac_input;
                                    _deviation = deviation;
                                    _hysteresis = hysteresis;
                                };

        bool isBatteryMode() { return _batteryMode; };

        void setShutdownMode(bool mode) {_shutdownMode = mode; };
        void setSelfTestMode(bool mode) {_selfTestMode = mode; };

        uint16_t getStatus() { return _status; };

        float getLastFaultInputVoltage() { return _last_fault_input_voltage; };

        void toggleBeeper() { _status ^= (uint16_t)1 << BEEPER_IS_ACTIVE; };

        float getBatteryLevel() { return _battery_level; };

        void toggleInverter(bool mode);

        void toggleOutput(bool mode);

        // connect or disconnect the mains
        void toggleInput(bool mode);

        void adjustOutput(RegulateMode mode = REGULATE_NONE);

        // write status flag. ATTENTION: this function should not be called from the timers
        void writeStatus(uint16_t nbit, bool value);

        bool readStatus(int nbit) { return bitRead(_status, nbit); };
    
    private:
        RMSSensor *_vac_in, *_vac_out;
        Sensor *_ac_out, *_v_bat;

        SimpleTimer *_beeper_timer;

        float _nominal_vac_input = INTERACTIVE_DEFAULT_INPUT_VOLTAGE;
        float _deviation = INTERACTIVE_INPUT_VOLTAGE_DEVIATION;
        float _hysteresis = INTERACTIVE_INPUT_VOLTAGE_HYSTERESIS;

        float _last_fault_input_voltage = 0;

        uint16_t _status = 0;

        bool _batteryMode = false;

        bool _shutdownMode = false;
        bool _selfTestMode = false;

        unsigned long _last_fail_time = 0;

        float _battery_level; 

};

#endif