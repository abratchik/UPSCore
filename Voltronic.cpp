#include "Voltronic.h"


Voltronic::Voltronic( HardwareSerial* stream, char protocol ) {
    _stream = stream;

    _param[PARAM_SELFTEST_MIN] = MIN_SELFTEST_DURATION;
    _param[PARAM_OUTPUT_FREQ] = INTERACTIVE_DEFAULT_FREQ;
    _param[PARAM_INTERNAL_TEMP] = DEFAULT_INTERNAL_TEMP;
    _param[PARAM_OUTPUT_VAC_NOMINAL] = INTERACTIVE_DEFAULT_INPUT_VOLTAGE;
    _param[PARAM_OUTPUT_AC_NOMINAL] = INTERACTIVE_MAX_AC_OUT;
    _param[PARAM_BATTERY_VDC_NOMINAL] = INTERACTIVE_MAX_V_BAT;
    _param[PARAM_DISPLAY_BRIGHTNESS_LEVEL] = DISPLAY_DEFAULT_BRIGHTNESS;
}

void Voltronic::begin(int baud_rate ) {
    _stream->begin(baud_rate);
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
                printPrompt(); 
                _stream->write(VOLTRONIC_DEFAULT_PROTOCOL); 
                writeEOL();
                break;
            case 'Q':
                if(_buf[1] == 'S') {

                    // universal but slower method 
                    // printParam("(%4.1f %4.1f %4.1f %3i %3.1f %3.1f %3.1f %b\n",
                    //     _param[PARAM_INPUT_VAC],
                    //     _param[PARAM_INPUT_FAULT_VAC],
                    //     _param[PARAM_OUTPUT_VAC],
                    //     (int)(_param[PARAM_OUTPUT_LOAD_LEVEL] * 100),
                    //     _param[PARAM_OUTPUT_FREQ],
                    //     _param[PARAM_BATTERY_VDC],
                    //     _param[PARAM_INTERNAL_TEMP],
                    //     _status
                    // );

                    _stream->write('(');
                    writeFloat(_param[PARAM_INPUT_VAC], 4, 1);
                    _stream->write(' ');
                    writeFloat(_param[PARAM_INPUT_FAULT_VAC], 4, 1);
                    _stream->write(' ');
                    writeFloat(_param[PARAM_OUTPUT_VAC], 4, 1);
                    _stream->write(' ');
                    writeInt( (int)(_param[PARAM_OUTPUT_LOAD_LEVEL] * 100), 3);
                    _stream->write(' ');
                    writeFloat(_param[PARAM_OUTPUT_FREQ], 3, 1);
                    _stream->write(' ');
                    writeFloat(_param[PARAM_BATTERY_VDC], 3, 1); 
                    _stream->write(' ');
                    writeFloat(_param[PARAM_INTERNAL_TEMP], 3, 1);
                    _stream->write(' ');
                    writeBin(_status);
                    writeEOL();
                }
                else if( _buf[1] == 'G' && _buf[2] == 'S' ) {
                    // TODO: support Grand Status
                    printPrompt(); 
                    _stream->write(_buf, _buf_ptr);
                    writeEOL();
                }
                else if( _buf[1] == 'R' && _buf[2] == 'I' ) {
                    printRatedInfo();
                }
                else if( _buf[1] == 'M' && _buf[2] == 'D' ) {

                    printParam("(%15s %7i %3.0f %3s %3.0f %3.0f %2i %3.1f",
                        PART_NUMBER,
                        RATED_VA,
                        (float)100.0F * ACTUAL_VA / RATED_VA,
                        "1/1",
                        INTERACTIVE_DEFAULT_INPUT_VOLTAGE,
                        _param[PARAM_OUTPUT_VAC_NOMINAL],
                        INTERACTIVE_NUM_CELLS,
                        INTERACTIVE_MAX_V_BAT_CELL
                    );

                    // _stream->write('(');
                    // printFixed(PART_NUMBER, 15);
                    // _stream->write(' ');
                    // writeInt(RATED_VA, 7);
                    // _stream->write(' ');
                    // writeFloat( (float)100.0F * ACTUAL_VA / RATED_VA, 3,0);     
                    // _stream->write(' ');
                    // printFixed("1/1", 3);
                    // _stream->write(' ');
                    // writeFloat( INTERACTIVE_DEFAULT_INPUT_VOLTAGE, 3,0);
                    // _stream->write(' ');
                    // writeFloat( _param[PARAM_OUTPUT_VAC_NOMINAL], 3, 0);
                    // _stream->write(' ');
                    // writeInt(INTERACTIVE_NUM_CELLS, 2);
                    // _stream->write(' ');
                    // writeFloat(INTERACTIVE_MAX_V_BAT_CELL, 4, 1);
                    // writeEOL();

                }
                else if( _buf[1] == 'M' && _buf[2] == 'F' ) {
                    _stream->write('(');
                    _stream->print(MANUFACTURER);
                    writeEOL();
                }
                else if( _buf[1] == 'B' && _buf[2] == 'V' ) {

                    // printParam("(%4.2f %2i %2i %i %3i\n", 
                    //     _param[PARAM_BATTERY_VDC],
                    //     INTERACTIVE_NUM_CELLS,
                    //     INTERACTIVE_NUM_BATTERY_PACKS,
                    //     (int) ( _param[PARAM_BATTERY_LEVEL] * 100 ),
                    //     (int)_param[PARAM_REMAINING_MIN]
                    // );

                    _stream->write('(');
                    writeFloat(_param[PARAM_BATTERY_VDC], 4, 2);
                    _stream->write(' ');
                    writeInt(INTERACTIVE_NUM_CELLS, 2);
                    _stream->write(' ');
                    writeInt(INTERACTIVE_NUM_BATTERY_PACKS, 2);
                    _stream->write(' ');
                    writeInt((int) ( _param[PARAM_BATTERY_LEVEL] * 100 ), 3);
                    _stream->write(' ');
                    writeInt((int)_param[PARAM_REMAINING_MIN], 3);
                    writeEOL();
                }
                else {
                    command_status = COMMAND_BEEPER_MUTE;
                }
                break;

            // Query UPS for rated information #4 (old)
            // case 'F':
            //    printRatedInfo();
            //    break;
            
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
                else {
                    command_status = COMMAND_TOGGLE_DISPLAY;
                }
                
                break;
            
            case 'R':
                // hard reset
                pinMode(RESET_PIN, OUTPUT);
                digitalWrite(RESET_PIN, LOW);
                break;

            case 'T':
                command_status = COMMAND_SELF_TEST;
                if(_buf[1] != 'L') {
                    _param[PARAM_SELFTEST_MIN] = max( parseFloat(1,2), MIN_SELFTEST_DURATION );
                }
                else {
                    //TODO: deep discharge test
                    ;
                }
                
                break;
            
            case 'I':
                
                printParam("#%15s %10s %10s\n", 
                    MANUFACTURER,
                    PART_NUMBER,
                    FIRMWARE_VERSION
                );
                // printPrompt();
                // printFixed(MANUFACTURER, 15);
                // _stream->write(' ');
                // printFixed(PART_NUMBER,10);
                // _stream->write(' ');
                // printFixed(FIRMWARE_VERSION,10);
                // writeEOL();
                break;

            case 'S':

                _param[PARAM_SHUTDOWN_MIN] = parseFloat(1,2);

                if(_buf[3] == 'R') 
                    _param[PARAM_RESTORE_MIN] = parseFloat(4,4);
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
                
                _sensor_ptr = (int) parseFloat(1,1);

                if( _buf[2] == 'P' ) {
                    _sensor_param = (int) parseFloat(3,1);

                    if( _buf[4] == 'V' ) {
                        _sensor_param_value = parseFloat(5,17);
                        command_status = COMMAND_TUNE_SENSOR;
                    }
                    else 
                        command_status = COMMAND_READ_SENSOR;
                }
                else
                    command_status = COMMAND_READ_SENSOR;

                break;

            case 'W':
                // undocumented case - save sensor params to EEPROM
                command_status = COMMAND_SAVE_SENSORS;
                break;

            default:
                // Not implemented
                printPrompt(); 
                _stream->write('N');
                writeEOL();
                break;

        }

    }

    // clear buffer
    _buf_ptr=0;
    memset(_buf, 0x0, COMMAND_BUFFER_SIZE);

    return command_status;
}

void Voltronic::writeEOL() {
    _stream->println();
}

void Voltronic::writeFloat( float val, int len, int dec ) {
    char * buf = malloc(len+1);
    memset(buf, 0x30, len+1);

    int digits = val * pow(10, dec);

    if( dec > 0 ) {
        for(int i=0; i<= len; i++) {
            if(i != dec ) {
                *(buf+len-i) = 0x30 + digits % 10;
                digits /= 10;
            }
            else {
                *(buf+len-i) = '.';
            }
        }
        _stream->write(buf, len+1);
    }
    else {
        for(int i=0; i < len; i++) {
            *(buf+len - i - 1) = 0x30 + digits % 10;
            digits /= 10;
        }
        _stream->write(buf, len);
    }

    
    free(buf);
}

void Voltronic::writeInt(int val, int len ) {
    writeFloat((float) val, len, 0);
}

void Voltronic::writeBin( uint8_t val) {
    for(int i=7; i>=0; i--) {
        _stream->write(0x30 + ((val >> i) & 0x01));
    }
}


float Voltronic::parseFloat(int startpos, int len) {
    char * buf = malloc(len);

    memcpy(buf, _buf + startpos, len);
    float result  = atof(buf);

    free(buf);

    return result;
}

void Voltronic::printFixed(const char* str, int len) {
    int ptr=0;
    while(str[ptr] != '\0' && ptr < len) {
        _stream->write(str[ptr]);
        ptr++;
    }
    for(int i=ptr; i < len; i++) {
        _stream->write(' ');
    } 
}

void Voltronic::printRatedInfo() {

    printParam("#%4.1f %3i %3.1f %3.1f\n", 
        _param[PARAM_OUTPUT_VAC_NOMINAL],
        (int)_param[PARAM_OUTPUT_AC_NOMINAL],
        _param[PARAM_BATTERY_VDC_NOMINAL],
        _param[PARAM_OUTPUT_FREQ_NOMINAL]
    );

    // printPrompt();
    // writeFloat(_param[PARAM_OUTPUT_VAC_NOMINAL], 4, 1);
    // _stream->write(' ');
    // writeInt((int)_param[PARAM_OUTPUT_AC_NOMINAL], 3);
    // _stream->write(' ');
    // writeFloat(_param[PARAM_BATTERY_VDC_NOMINAL], 3, 1);
    // _stream->write(' ');
    // writeFloat(_param[PARAM_OUTPUT_FREQ_NOMINAL], 3, 1);
    // writeEOL();
}

void Voltronic::printSensorParams( float offset, float scale,  float value = 0) {   
    printParam("(%i %.5f %.5f %f\n", _sensor_ptr, offset, scale, value);
}

void Voltronic::printChargerParams(float kp, float ki, float kd, float cv, float cc, float v, float c, bool chrg, int mode, float err, int output) {
    _stream->write('(');
    _stream->print(_sensor_ptr);
    _stream->write(' ');
    _stream->print(kp);
    _stream->write(' ');
    _stream->print(ki);
    _stream->write(' ');
    _stream->print(kd);
    _stream->write(' ');
    _stream->print(cc);
    _stream->write(' ');
    _stream->print(cv);
    _stream->write(' ');
    _stream->print(c);
    _stream->write(' ');
    _stream->print(v);
    _stream->write(' ');    
    _stream->print(chrg);
    _stream->write(' ');
    _stream->print(mode);
    _stream->write(' ');
    _stream->print(err);
    _stream->write(' ');
    _stream->print(output);
    writeEOL();
}

void Voltronic::printPartModel() {
   printPrompt(); 
   _stream->println(PART_MODEL);
}

void Voltronic::printPrompt() {
    _stream->write('#');
}

void Voltronic::printParam(const char* fmt, ...) {
    va_list args;
    int wid, prec;

    va_start(args, fmt); 

    for(int index = 0; *(fmt + index) != '\0'; index++) {
         // read format
         if(*(fmt + index) == '%') {
            index++;
            wid = get_width_modifier(fmt + index, &index);
            prec = get_precision_modifier(fmt + index, &index);
            switch(*(fmt + index)) {
                case 'b':
                    writeBin( (int) va_arg(args, int) );
                    break;
                case 'i':   
                    if(wid)
                        writeInt( (int) va_arg(args, int), wid );
                    else
                        _stream->print((int) va_arg(args, int));
                    break;

                case 'f':
                    if( ( wid > 0 ) && ( prec >= 0 ) ) {
                        writeFloat((float) va_arg(args, double), wid, prec);
                    }
                    else if( ( prec > 0 ) && ( wid == 0 ) ) {
                        _stream->print((float) va_arg(args, double), prec);
                    }
                    else {
                        _stream->print((float) va_arg(args, double));
                    }
                    break;
                case 's':
                    if( wid > 0 ) {
                        printFixed((const char *) va_arg(args, const char*), wid);
                    }
                    else {
                        _stream->print((const char *) va_arg(args, const char*));
                    }
                    break;
                default:
                    _stream->print(*(fmt + index));
                    break;    
            }
         }
         else
            _stream->write(*(fmt + index));
    }

    va_end(args);

}

int Voltronic::get_width_modifier(const char* modifier, int *index) {
    int value = 0;

    while(*modifier >= '0' && *modifier <= '9') {
        (*index)++;

        value *= 10;
        value += (*modifier - '0');
        modifier++;
    }

    return value;
}

int Voltronic::get_precision_modifier(const char* modifier, int *index) {
    int value = 0;
    
    if ( *modifier != '.' )
        return (-1);

    modifier++;
    (*index)++;

    if ( *modifier <= '0' || *modifier > '9' ) {
        if (*modifier == '0') 
            (*index)++;
        return (0);
    }

    while( *modifier >= '0' && *modifier <= '9' ) {
        (*index)++;

        value *= 10;
		value += (*modifier - '0');
		modifier++;
    }

    return value;
}
