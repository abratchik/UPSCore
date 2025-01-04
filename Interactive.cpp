#include "Interactive.h"

Interactive::Interactive( Sensor *vac_in, Sensor *vac_out, Sensor *ac_out, Sensor *v_bat) {
    // link to sensors
    _vac_in = vac_in;
    _vac_out = vac_out;
    _ac_out = ac_out;
    _v_bat = v_bat;

    // set relays to output
    pinMode(INTERACTIVE_INPUT_RLY_OUT, OUTPUT);
    pinMode(INTERACTIVE_OUTPUT_RLY_OUT, OUTPUT);
    pinMode(INTERACTIVE_LEFT_RLY_OUT, OUTPUT);
    pinMode(INTERACTIVE_RIGHT_RLY_OUT, OUTPUT);
    pinMode(INTERACTIVE_INVERTER_OUT, OUTPUT);

    // type of the UPS
    bitSet( _status, LINE_INTERACTIVE );
    // beeper enabled at the start
    bitSet( _status, BEEPER_IS_ACTIVE );
}

RegulateStatus Interactive::regulate() {

    bool self_test = bitRead(_status, SELF_TEST);
    
    _battery_level = (_v_bat->reading() - INTERACTIVE_MIN_V_BAT) / INTERACTIVE_V_BAT_DELTA ;

    if(_battery_level < 0 ) {
        bitSet(_status, BATTERY_DEAD_FLAG );
    }
    else 
        bitClear(_status, BATTERY_DEAD_FLAG );

    if(_battery_level < INTERACTIVE_BATTERY_LOW ) {
        self_test = false;
        toggleSelfTest(false);
        bitSet(_status, BATTERY_LOW);
    }
    else {
        bitClear(_status, BATTERY_LOW);
    }

    if( bitRead(_status, SHUTDOWN_ACTIVE) ) {
        return REGULATE_STATUS_SHUTDOWN;
    }

    float ac_out = _ac_out->reading();

    // If output is disconnected but the load is present then it may 
    // point at the defective output relay or the load sensor
    if(!bitRead( _status, OUTPUT_RELAY_FLAG ) && ( ac_out > INTERACTIVE_MIN_AC_OUT ) ) 
        bitSet(_status, UNUSUAL_FLAG);
    else
        bitClear(_status, UNUSUAL_FLAG);

    // overload protection
    if(ac_out > INTERACTIVE_MAX_AC_OUT)
        bitSet(_status, OVERLOAD_FLAG); 

    float vac_input = _vac_in->reading();

    float abs_deviation =  abs(_nominal_vac_input - vac_input);
    float nominal_deviation = _deviation * _nominal_vac_input;

    float nominal_hysteresis = _hysteresis * _nominal_vac_input;       

    // wrong output voltage protection after inverter
    if(_batteryMode && _vac_out->ready()) {
        float vac_output = _vac_out->reading();
        if( abs(_nominal_vac_input - vac_output) > nominal_deviation ) 
            bitSet(_status, UPS_FAULT);
    }

    // if the state is overload or output voltage is wrong, no regulation, need cold reset.
    if(bitRead( _status, OVERLOAD_FLAG) || bitRead( _status, UPS_FAULT ) || bitRead( _status, BATTERY_DEAD_FLAG ) ) 
        return REGULATE_STATUS_ERROR;
    
    // input voltage is far off the regulation limits (X2)
    bool utility_fail = abs_deviation > 2 * nominal_deviation - nominal_hysteresis * ( _batteryMode? 1 : -1 );

    if(utility_fail){
        self_test = false;
        if(!_batteryMode) _last_fault_input_voltage = vac_input;
        bitSet(_status, UTILITY_FAIL);
    }

    if( utility_fail || self_test )  {

        disconnectInput();

        adjustOutput(REGULATE_NONE);

        return REGULATE_STATUS_FAIL;

    }
        
    // input voltage is within the regulation limits 
    bitClear(_status, UTILITY_FAIL);

    // regulate
    if( abs_deviation > nominal_deviation - nominal_hysteresis * (bitRead(_status, REGULATE_FLAG)? 1 : -1 ) ) {
        // input voltage deviation exceeds the limit

        if(vac_input > _nominal_vac_input ) 
            // input voltage is higher than limit, step down
            adjustOutput(REGULATE_DOWN);
        else 
            // input voltage is lower than limit, step up
            adjustOutput(REGULATE_UP);

    }
    else {

        // input voltage is within the limits, pass on to the load
        adjustOutput(REGULATE_NONE);
    }

    return REGULATE_STATUS_SUCCESS;  

}

void Interactive::startInverter() {
    digitalWrite(INTERACTIVE_INVERTER_OUT, HIGH);
    _batteryMode = true;
}

void Interactive::stopInverter() {
    digitalWrite(INTERACTIVE_INVERTER_OUT, LOW);
    _batteryMode = false;
}

void Interactive::connectOutput() {
    digitalWrite(INTERACTIVE_OUTPUT_RLY_OUT, HIGH);
    bitSet(_status, OUTPUT_RELAY_FLAG);
}

void Interactive::disconnectOutput() {
    digitalWrite(INTERACTIVE_OUTPUT_RLY_OUT, LOW);
    bitClear(_status, OUTPUT_RELAY_FLAG);
}

void Interactive::connectInput() {
    digitalWrite(INTERACTIVE_INPUT_RLY_OUT, HIGH);
    bitSet(_status, INPUT_RELAY_FLAG);
}

void Interactive::disconnectInput() {
    digitalWrite(INTERACTIVE_INPUT_RLY_OUT, LOW);
    bitClear(_status, INPUT_RELAY_FLAG);
}

void Interactive::adjustOutput(RegulateMode mode) {
    switch(mode) {
        case REGULATE_UP:
            bitSet(_status, REGULATE_FLAG);
            digitalWrite(INTERACTIVE_LEFT_RLY_OUT, LOW);
            digitalWrite(INTERACTIVE_RIGHT_RLY_OUT, HIGH);
            break;

        case REGULATE_DOWN:
            bitSet(_status, REGULATE_FLAG);
            digitalWrite(INTERACTIVE_LEFT_RLY_OUT, HIGH);
            digitalWrite(INTERACTIVE_RIGHT_RLY_OUT, LOW);
            break;

        default:
            bitClear(_status, REGULATE_FLAG);
            digitalWrite(INTERACTIVE_LEFT_RLY_OUT, LOW);
            digitalWrite(INTERACTIVE_RIGHT_RLY_OUT, LOW);
            break;
    }
}

void Interactive::toggleSelfTest(bool active) {
    if(active)
        bitSet(_status, SELF_TEST);
    else
        bitClear(_status, SELF_TEST);
}

void Interactive::toggleOutput(bool shutdown) {
    if(shutdown)
        bitSet(_status, SHUTDOWN_ACTIVE);
    else
        bitClear(_status, SHUTDOWN_ACTIVE);
}
