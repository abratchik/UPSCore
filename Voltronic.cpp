#include "Voltronic.h"


Voltronic::Voltronic( Stream* stream) {
    _stream = stream;

    _param[PARAM_SELFTEST_MIN] = MIN_SELFTEST_DURATION;
    _param[PARAM_OUTPUT_FREQ] = INTERACTIVE_DEFAULT_FREQ;
    _param[PARAM_INTERNAL_TEMP] = DEFAULT_INTERNAL_TEMP;
    _param[PARAM_OUTPUT_VAC_NOMINAL] = INTERACTIVE_DEFAULT_INPUT_VOLTAGE;
    _param[PARAM_OUTPUT_AC_NOMINAL] = INTERACTIVE_MAX_AC_OUT;
    _param[PARAM_BATTERY_VDC_NOMINAL] = INTERACTIVE_MAX_V_BAT;

#ifndef DISPLAY_TYPE_NONE    
    _param[PARAM_DISPLAY_BRIGHTNESS_LEVEL] = DISPLAY_DEFAULT_BRIGHTNESS;
#endif

}

char Voltronic::process() {

    if(_stream->available() ) {
        int ch = _stream->read();  

        if(ch == '\r') {
            _buf[_buf_ptr] = '\0';
            return ch;
        }
        else {
            _buf[_buf_ptr] = (char)lowByte(ch);
        }

        _buf_ptr++;

        if( _buf_ptr >= COMMAND_BUFFER_SIZE ) 
            _buf_ptr = 0;
    }

    return '\0';    
}

ExecuteCommand Voltronic::executeCommand() {

    int command_status = COMMAND_NONE;
    
    if( _buf[0] != '#' && _buf[0] != '(') {

        switch(_buf[0]) {
            case 'M':
                _stream->write(VOLTRONIC_PROMPT); 
                _stream->write(VOLTRONIC_DEFAULT_PROTOCOL); 
                _stream->println();
                break;
            case 'Q':
                if(_buf[1] == 'S') {

                    ex_printf_to_stream(_stream, "(%4.1f %4.1f %4.1f %3i %3.1f %3.1f %3.1f %b\r\n",
                        _param[PARAM_INPUT_VAC],
                        _param[PARAM_INPUT_FAULT_VAC],
                        _param[PARAM_OUTPUT_VAC],
                        (int)(_param[PARAM_OUTPUT_LOAD_LEVEL] * 100),
                        _param[PARAM_OUTPUT_FREQ],
                        _param[PARAM_BATTERY_VDC],
                        _param[PARAM_INTERNAL_TEMP],
                        _status
                    );

                }
                else if( _buf[1] == 'G' && _buf[2] == 'S' ) {
                    // TODO: support Grand Status
                    _stream->write(VOLTRONIC_PROMPT); 
                    _stream->write(_buf, _buf_ptr);
                    _stream->println();
                }
                else if( _buf[1] == 'R' && _buf[2] == 'I' ) {
                    ex_printf_to_stream(_stream, "#%4.1f %3i %3.1f %3.1f\r\n", 
                        _param[PARAM_OUTPUT_VAC_NOMINAL],
                        (int)_param[PARAM_OUTPUT_AC_NOMINAL],
                        _param[PARAM_BATTERY_VDC_NOMINAL],
                        _param[PARAM_OUTPUT_FREQ_NOMINAL]
                    );
                }
                else if( _buf[1] == 'M' && _buf[2] == 'D' ) {

                    ex_printf_to_stream(_stream, "(%15S %7i %3.0f %3s %3.0f %3.0f %2i %3.1f\r\n",
                        PART_NUMBER,
                        RATED_VA,
                        (float)100.0F * ACTUAL_VA / RATED_VA,
                        "1/1",
                        INTERACTIVE_DEFAULT_INPUT_VOLTAGE,
                        _param[PARAM_OUTPUT_VAC_NOMINAL],
                        INTERACTIVE_NUM_CELLS,
                        INTERACTIVE_MAX_V_BAT_CELL
                    );

                }
                else if( _buf[1] == 'M' && _buf[2] == 'F' ) {
                    _stream->write('(');
                    ex_print_str_to_stream(_stream, MANUFACTURER, true);
                    _stream->println();
                }
                else if( _buf[1] == 'B' && _buf[2] == 'V' ) {

                    ex_printf_to_stream(_stream, "(%4.2f %2i %2i %i %3i\r\n", 
                        _param[PARAM_BATTERY_VDC],
                        INTERACTIVE_NUM_CELLS,
                        INTERACTIVE_NUM_BATTERY_PACKS,
                        (int) ( _param[PARAM_BATTERY_LEVEL] * 100 ),
                        (int)_param[PARAM_REMAINING_MIN]
                    );

                }
                else {
                    command_status = COMMAND_BEEPER_MUTE;
                }
                break;
 
#ifndef DISPLAY_TYPE_NONE            
            case 'D':
                // 'undocumented' case - change the display brightness
                // format is DN where N is from 0 to 9 - sets the brightness level.
                // 0 level means display is off
                // if N is omitted the display will be switched on or off using max brightness as default
                
                if(isDigit(_buf[1])) {
                    int lvl = (char)(_buf[1] - '0');
                    if( lvl <= DISPLAY_MAX_BRIGHTNESS ) {
                        _param[PARAM_DISPLAY_BRIGHTNESS_LEVEL] = lvl;
                        command_status = COMMAND_SET_BRIGHTNESS;
                    }   
                }
                else if (_buf[1] == 'M')
                {
                    command_status = COMMAND_TOGGLE_DISPLAY_MODE;
                }
                else {
                    command_status = COMMAND_TOGGLE_DISPLAY;
                }
                
                break;
#endif            
            case 'R':
                // hard reset
                pinMode(RESET_PIN, OUTPUT);
                digitalWrite(RESET_PIN, LOW);
                break;

            case 'T':
                command_status = COMMAND_SELF_TEST;
                if(_buf[1] != 'L') {
                    _param[PARAM_SELFTEST_MIN] = max( ex_parse_float(_buf, 1,2), MIN_SELFTEST_DURATION );
                }
                else {
                    //TODO: deep discharge test
                    ;
                }
                
                break;
            
            case 'I':
                
                ex_printf_to_stream(_stream, "#%15S %10S %10S\r\n", 
                    MANUFACTURER,
                    PART_NUMBER,
                    FIRMWARE_VERSION
                );

                break;

            case 'S':

                _param[PARAM_SHUTDOWN_MIN] = ex_parse_float(_buf, 1,2);

                if(_buf[3] == 'R') 
                    _param[PARAM_RESTORE_MIN] = ex_parse_float(_buf, 4,4);
                else
                    _param[PARAM_RESTORE_MIN] = 0;

                if(_param[PARAM_SHUTDOWN_MIN] > 0)
                    command_status = COMMAND_SHUTDOWN;

                break;

            case 'C':
                switch(_buf[1]) {
                    case 'T':
                        command_status = COMMAND_SELF_TEST_CANCEL;
                        break;
                    case 'S':
                    default:
                        command_status = COMMAND_SHUTDOWN_CANCEL;
                        break;
                }
                break;

            case 'V':
                // undocumented case - allows to tune or read sensors and charger params
                // format: VNPMVKKKKKKKKKKKKKKKKK, where
                // N - id of the sensor (0..4)
                // M - can be 0 (scale) or 1 (offset). PM can be omitted - then sensor params are printed
                // K - float value to be set (17 symbols). Can be omitted
                
                _sensor_ptr = (uint8_t) ex_parse_float(_buf, 1,1);
                switch( _buf[2]) {
                    case 'P':
                        _sensor_param = (uint8_t) ex_parse_float(_buf, 3,1);

                        if( _buf[4] == 'V' ) {
                            _sensor_param_value = ex_parse_float(_buf, 5,17);
                            command_status = COMMAND_TUNE_SENSOR;
                        }
                        else 
                            command_status = COMMAND_READ_SENSOR;
                        break;
                    case 'D':
                        command_status = COMMAND_DUMP_SENSOR;
                        break;
                    default:
                        command_status = COMMAND_READ_SENSOR;
                        break;

                }

                break;

            case 'W':
                // undocumented case - save sensor params to EEPROM
                command_status = COMMAND_SAVE_SENSORS;
                break;

            default:
                // Not implemented
                _stream->write(VOLTRONIC_PROMPT); 
                _stream->write('N');
                _stream->println();
                break;

        }

    }

    // clear buffer
    _buf_ptr=0;
    memset(_buf, 0x0, COMMAND_BUFFER_SIZE);

    return command_status;
}


