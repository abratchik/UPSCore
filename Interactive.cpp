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
    pinMode(INTERACTIVE_ERROR_OUT, OUTPUT);

    // type of the UPS
    writeStatus( LINE_INTERACTIVE, true );
    // beeper enabled at the start
    writeStatus( BEEPER_IS_ACTIVE, true );
}

RegulateStatus Interactive::regulate(unsigned long ticks) {
    
    // read the sensors
    _battery_level = max(min((_v_bat->reading() - INTERACTIVE_MIN_V_BAT) / INTERACTIVE_V_BAT_DELTA, 1.0F), 0.0F) ;
    float ac_out = _ac_out->reading();
    float vac_input = _vac_in->reading();

    float abs_deviation =  abs(_nominal_vac_input - vac_input);
    float nominal_deviation = _deviation * _nominal_vac_input;
    float nominal_hysteresis = _hysteresis * _nominal_vac_input;  

    writeStatus(BATTERY_DEAD, _v_bat->reading() < INTERACTIVE_MIN_V_BAT );
    writeStatus(BATTERY_LOW, _battery_level < INTERACTIVE_BATTERY_LOW );

    // If output is disconnected but the load is present then it may 
    // point at the defective output relay or the load sensor
    writeStatus(UNUSUAL_STATE, !readStatus( OUTPUT_CONNECTED ) && ( ac_out > INTERACTIVE_MIN_AC_OUT ) );

    // overload protection
    writeStatus(OVERLOAD, ac_out > INTERACTIVE_MAX_AC_OUT);
     
    // input voltage is far off the regulation limits (X2)
    writeStatus(UTILITY_FAIL, abs_deviation > 2 * (nominal_deviation - nominal_hysteresis * ( _batteryMode? 1 : - 1 )));

    // stop self-test if the battery is low
    writeStatus(SELF_TEST, _selfTestMode && !readStatus(BATTERY_LOW) );

    // wrong output voltage protection after inverter
    if(_batteryMode ) {
        float out_deviation = _vac_out->reading() - _nominal_vac_input;

        if( out_deviation > nominal_deviation || 
            ( ( -out_deviation > nominal_deviation ) && ( abs(ticks - _last_time) > INVERTER_GRACE_PERIOD ) ) )   
            writeStatus(UPS_FAULT, true);
    }

    _last_time = ticks;
    

    if(readStatus(UTILITY_FAIL)) {
        _last_fail_time = ticks;

        writeStatus(SELF_TEST, false);
        if(!_batteryMode) {
            _last_fault_input_voltage = vac_input;
        }
    }

    if(_shutdownMode) {
        // writeStatus(UTILITY_FAIL, utility_fail);
        return update_state(REGULATE_STATUS_SHUTDOWN);
    }
    else {
        if( readStatus( SHUTDOWN_ACTIVE ) ) {
            return update_state(REGULATE_STATUS_WAKEUP);
        }
    }

    // if the state is overload or output voltage is wrong, no regulation, need cold reset.
    // if(readStatus( OVERLOAD ) || readStatus( UPS_FAULT ) ) {
    if( _status & (( 1U << OVERLOAD ) | ( 1U << UPS_FAULT )) )  {
        return update_state(REGULATE_STATUS_ERROR);
    }

    if( ( _status & (( 1U << UTILITY_FAIL ) | ( 1U << SELF_TEST )) ) || (ticks - _last_fail_time < TIMER_ONE_SEC * 2) )  {

        toggleInput(false);

        adjustOutput(REGULATE_NONE);

        if(readStatus(BATTERY_DEAD)) 
            return update_state(REGULATE_STATUS_ERROR);
        else 
            return update_state(REGULATE_STATUS_FAIL);
    }
        
    // input voltage is within the regulation limits 

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

    return update_state(REGULATE_STATUS_SUCCESS);  

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

void Interactive::toggleError(bool mode) {
    digitalWrite(INTERACTIVE_ERROR_OUT, mode);
}

void Interactive::adjustOutput(RegulateMode mode) {

    if(mode) {
        writeStatus(REGULATED, true);
        digitalWrite(INTERACTIVE_LEFT_RLY_OUT, mode == REGULATE_DOWN ? HIGH : LOW);
        digitalWrite(INTERACTIVE_RIGHT_RLY_OUT, mode == REGULATE_UP ? HIGH : LOW);
    }
    else {
        writeStatus(REGULATED, false);
        digitalWrite(INTERACTIVE_LEFT_RLY_OUT, LOW);
        digitalWrite(INTERACTIVE_RIGHT_RLY_OUT, LOW);
    }
    
}

void Interactive::writeStatus(uint16_t nbit, bool value) {
    if(value) 
        bitSet(_status, nbit);
    else
        bitClear(_status, nbit);
}

void Interactive::sleep(uint32_t timeout) {

    wdt_disable();


    ADCSRA &= ~ (1 << ADEN);    // Disable ADC
    ACSR |= (1 << ACD);         // Disable comparator

    while (timeout > 0) {

        wdt_enable(WDTO_250MS);                   // Enable WDT
        WDTCSR |= (1 << WDIE);                    // Режим ISR+RST

        wdt_reset();                            // Reset WDT  
        set_sleep_mode(SLEEP_MODE);   
        sleep_enable(); 				        // Enable sleep mode
        sleep_cpu(); 				            // Put the CPU to sleep
        sleep_disable();                        // Disable sleep mode
        wdt_disable();                          // Disable WDT
        wdt_reset();                            // Reset WDT

        timeout --;
    }
    
    ADCSRA |= (1 << ADEN);      // Enable ADC
    ACSR &= ~ (1 << ACD);       // Enable comparator
    
    wdt_enable(WDTO_2S);

}

ISR(WDT_vect) {                     
    for(int i=0; i<1000;i++);
}
