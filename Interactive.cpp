#include "Interactive.h"

Interactive::Interactive( RMSSensor *vac_in, RMSSensor *vac_out, Sensor *ac_out, Sensor *v_bat) {
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
    writeStatus( LINE_INTERACTIVE, true );
    // beeper enabled at the start
    writeStatus( BEEPER_IS_ACTIVE, true );
}

RegulateStatus Interactive::regulate(unsigned long ticks) {
    
    _battery_level = max(min((_v_bat->reading() - INTERACTIVE_MIN_V_BAT) / INTERACTIVE_V_BAT_DELTA, 1.0F), 0.0F) ;
    writeStatus(SELF_TEST, _selfTestMode);
    writeStatus(SHUTDOWN_ACTIVE, _shutdownMode);
    writeStatus(BATTERY_DEAD, _v_bat->reading() < INTERACTIVE_MIN_V_BAT );

    bool self_test = readStatus(SELF_TEST);

    bool battery_low = (_battery_level < INTERACTIVE_BATTERY_LOW);

    if( battery_low ) {
        self_test = false;
        writeStatus(SELF_TEST, false);
    }

    writeStatus(BATTERY_LOW, battery_low);

    if( readStatus( SHUTDOWN_ACTIVE ) ) {
        return REGULATE_STATUS_SHUTDOWN;
    }

    float ac_out = _ac_out->reading();

    // If output is disconnected but the load is present then it may 
    // point at the defective output relay or the load sensor

    writeStatus(UNUSUAL_STATE, !readStatus( OUTPUT_CONNECTED ) && ( ac_out > INTERACTIVE_MIN_AC_OUT ) );

    // overload protection
    if(ac_out > INTERACTIVE_MAX_AC_OUT)
        writeStatus(OVERLOAD, true); 

    float vac_input = _vac_in->reading();

    float abs_deviation =  abs(_nominal_vac_input - vac_input);
    float nominal_deviation = _deviation * _nominal_vac_input;

    float nominal_hysteresis = _hysteresis * _nominal_vac_input;       

    // wrong output voltage protection after inverter
    if(_batteryMode ) {
        float deviation = _vac_out->reading() - _nominal_vac_input;

        if( deviation > nominal_deviation || 
            (-deviation > nominal_deviation && abs(ticks - _last_time) > INVERTER_GRACE_PERIOD ))   
            writeStatus(UPS_FAULT, true);
    }

    _last_time = ticks;

    // if the state is overload or output voltage is wrong, no regulation, need cold reset.
    if(readStatus( OVERLOAD ) || readStatus( UPS_FAULT ) || readStatus( BATTERY_DEAD ) ) 
        return REGULATE_STATUS_ERROR;
    
    // input voltage is far off the regulation limits (X2)
    bool utility_fail = abs_deviation > 2 * (nominal_deviation - nominal_hysteresis * ( _batteryMode? 1 : - 1 ));

    if(utility_fail){
        self_test = false;
        _last_fail_time = ticks;

        writeStatus(SELF_TEST, false);
        if(!_batteryMode) {
            _last_fault_input_voltage = vac_input;
        }
        writeStatus(UTILITY_FAIL, true);
    }

    if( utility_fail || self_test || (ticks - _last_fail_time < TIMER_ONE_SEC * 2) )  {

        toggleInput(false);

        adjustOutput(REGULATE_NONE);

        return REGULATE_STATUS_FAIL;

    }
        
    // input voltage is within the regulation limits 
    writeStatus(UTILITY_FAIL, false);

    // regulate
    if( abs_deviation > nominal_deviation - nominal_hysteresis * (bitRead(_status, REGULATED)? 1 : -1 ) ) {
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

void Interactive::toggleInverter(bool mode) {
    digitalWrite(INTERACTIVE_INVERTER_OUT, mode);
    _batteryMode = mode;
}

void Interactive::toggleOutput( bool mode ) {
    digitalWrite(INTERACTIVE_OUTPUT_RLY_OUT, mode);
    writeStatus(OUTPUT_CONNECTED, mode);
}


void Interactive::toggleInput(bool mode) {
    digitalWrite(INTERACTIVE_INPUT_RLY_OUT, mode);
    writeStatus(INPUT_CONNECTED, mode);
}

void Interactive::adjustOutput(RegulateMode mode) {
    switch(mode) {
        case REGULATE_UP:
            writeStatus(REGULATED, true);
            digitalWrite(INTERACTIVE_LEFT_RLY_OUT, LOW);
            digitalWrite(INTERACTIVE_RIGHT_RLY_OUT, HIGH);
            break;

        case REGULATE_DOWN:
            writeStatus(REGULATED, true);
            digitalWrite(INTERACTIVE_LEFT_RLY_OUT, HIGH);
            digitalWrite(INTERACTIVE_RIGHT_RLY_OUT, LOW);
            break;

        default:
            writeStatus( REGULATED, false);
            digitalWrite(INTERACTIVE_LEFT_RLY_OUT, LOW);
            digitalWrite(INTERACTIVE_RIGHT_RLY_OUT, LOW);
            break;
    }
}

void Interactive::writeStatus(uint16_t nbit, bool value) {
    if(value) 
        bitSet(_status, nbit);
    else
        bitClear(_status, nbit);
}