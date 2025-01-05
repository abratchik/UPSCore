#include "Voltronic.h"


Voltronic::Voltronic( HardwareSerial* stream, char protocol ) {
    _stream = stream;
    _protocol = protocol;
}

void Voltronic::begin(int baud_rate ) {
    _stream->begin(baud_rate);
}

char Voltronic::process() {

    if(_stream->available() ) {
        int ch = _stream->read();  

        if(ch == '\r') {
            _buf[_ptr] = '\0';
            _ptr = 0;
            return ch;
        }
        else {
            _buf[_ptr] = (char)lowByte(ch);
        }

        _ptr++;

        if( _ptr >= COMMAND_BUFFER_SIZE ) 
            _ptr = 0;
    }

    return '\0';    
}

ExecuteCommand Voltronic::executeCommand() {

    int command_status = COMMAND_NONE;

    if(_buf[0] == 'M') {
        _stream->write(_protocol); 
        writeEOL();
    }
    else {
        switch(_protocol) {
            case 'V':
                switch(_buf[0]) {
                    case 'Q':
                        if(_buf[1] == 'S') {
                            _stream->write('(');
                            writeFloat(_in_v, 4, 1);
                            _stream->write(' ');
                            writeFloat(_in_flt_v, 4, 1);
                            _stream->write(' ');
                            writeFloat(_out_v, 4, 1);
                            _stream->write(' ');
                            writeInt(_load_lvl, 3);
                            _stream->write(' ');
                            writeFloat(_out_f, 3, 1);
                            _stream->write(' ');
                            writeFloat(_bat_v, 3, 1); 
                            _stream->write(' ');
                            writeFloat(_int_t, 3, 1);
                            _stream->write(' ');
                            writeBin(_status);
                            writeEOL();
                        }
                        else if( _buf[1] == 'G' && _buf[2] == 'S' ) {
                            // TODO: support Grand Status
                            _stream->write(_buf);
                            writeEOL();
                        }
                        else if( _buf[1] == 'R' && _buf[2] == 'I' ) {
                            printRatedInfo();
                        }
                        else if( _buf[1] == 'M' && _buf[2] == 'D' ) {
                            _stream->write('(');
                            printFixed(PART_NUMBER, 15);
                            _stream->write(' ');
                            writeInt(RATED_VA, 7);
                            _stream->write(' ');
                            writeFloat( (float)100.0F * ACTUAL_VA / RATED_VA, 3,0);     
                            _stream->write(' ');
                            printFixed("1/1", 3);
                            _stream->write(' ');
                            writeFloat( INTERACTIVE_DEFAULT_INPUT_VOLTAGE, 3,0);
                            _stream->write(' ');
                            writeFloat( _out_v_nom, 3, 0);
                            _stream->write(' ');
                            writeInt(INTERACTIVE_NUM_CELLS, 2);
                            _stream->write(' ');
                            writeFloat(INTERACTIVE_MAX_V_BAT_CELL, 4, 1);
                            writeEOL();

                        }
                        else if( _buf[1] == 'M' && _buf[2] == 'F' ) {
                            _stream->write('(');
                            _stream->print(MANUFACTURER);
                            writeEOL();
                        }
                        else if( _buf[1] == 'B' && _buf[2] == 'V' ) {
                            _stream->write('(');
                            writeFloat(_bat_v, 4, 2);
                            _stream->write(' ');
                            writeInt(INTERACTIVE_NUM_CELLS, 2);
                            _stream->write(' ');
                            writeInt(INTERACTIVE_NUM_BATTERY_PACKS, 2);
                            _stream->write(' ');
                            writeInt(_battery_lvl, 3);
                            _stream->write(' ');
                            writeInt(_remaining_min, 3);
                            writeEOL();
                        }
                        else {
                            command_status = COMMAND_BEEPER_MUTE;
                        }
                        break;

                    case 'F':
                        // Query UPS for rated information #4 (old)
                        printRatedInfo();
                        break;
                    
                    case 'D':
                        // 'undocumented' case - change the display brightness
                        // format is DN where N is from 0 to 9 - sets the brightness level.
                        // 0 level means display is off
                        // if N is omitted the display will be switched on or off using max brightness as default
                        
                        if(isDigit(_buf[1])) {
                            int lvl = (char)(_buf[1] - '0');
                            if( lvl <= DISPLAY_MAX_BRIGHTNESS ) {
                                _brightness_lvl = lvl;
                                command_status = COMMAND_SET_BRIGHTNESS;
                            }   
                        }
                        else {
                            command_status = COMMAND_TOGGLE_DISPLAY;
                        }
                        
                        break;

                    case 'T':
                        command_status = COMMAND_SELF_TEST;
                        if(_buf[1] != 'L') {
                            _selftest_min = max( parseFloat(1,2), MIN_SELFTEST_DURATION );
                        }
                        else {
                            //TODO: deep discharge test
                            ;
                        }
                        
                        break;
                    
                    case 'I':
                        _stream->write('#');
                        printFixed(MANUFACTURER, 15);
                        _stream->write(' ');
                        printFixed(PART_NUMBER,10);
                        _stream->write(' ');
                        printFixed(FIRMWARE_VERSION,10);
                        writeEOL();
                        break;

                    case 'S':

                        _shutdown_min = parseFloat(1,2);

                        if(_buf[3] == 'R') 
                            _restore_min = (int) parseFloat(4,4);
                        else
                            _restore_min = 0;

                        if(_shutdown_min > 0)
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
                        // undocumented case - allows to tune sensor params
                        // format: VNPMVKKKKKKKKKKKKKKKKK, where
                        // N - id of the sensor (0..4)
                        // M - can be 0 (scale) or 1 (offset) 
                        // K - float value to be set (17 symbols). 

                        if(_buf[2] == 'P' && _buf[4] == 'V') {
                            _sensor_ptr = (int) parseFloat(1,1);
                            _sensor_param = (int) parseFloat(3,1);
                            _sensor_value = parseFloat(5,17);
                            command_status = COMMAND_TUNE_SENSOR;
                        }
                        break;

                    default:
                        _stream->write(_buf);
                        writeEOL();
                        break;

                }
                break;
            default:
                // P, T protocols not implemented
                _stream->write('N');
                writeEOL();
                break;

        }
    }

    // clear buffer
    _ptr=0;
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
    _stream->write('#');
    writeFloat(_out_v_nom, 4, 1);
    _stream->write(' ');
    writeInt(_out_c_nom, 3);
    _stream->write(' ');
    writeFloat(_bat_v_nom, 3, 1);
    _stream->write(' ');
    writeFloat(_out_f_nom, 3, 1);
    writeEOL();
}