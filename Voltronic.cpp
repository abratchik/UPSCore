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

CommandStatus Voltronic::executeCommandBuffer() {

    int command_status = COMMAND_NONE;

    if(_buf[0] == 'M') {
        _stream->write(_protocol); 
        writeEOL();
    }
    else {
        switch(_protocol) {
            case 'T':
                switch(_buf[0]) {
                    case 'Q':
                        if(_buf[1] == 'S') {
                            ;
                        }
                        else {
                            ;
                        }
                        break;
                    default:
                        _stream->write('N');
                        writeEOL();
                        break;
                }
                break;
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
                        else {
                            command_status = COMMAND_BEEPER_MUTE;
                        }
                        break;

                    case 'F':
                        _stream->write('#');
                        writeFloat(_out_v_nom, 4, 1);
                        _stream->write(' ');
                        writeInt(_out_c_nom, 3);
                        _stream->write(' ');
                        writeFloat(_bat_v_nom, 3, 1);
                        _stream->write(' ');
                        writeFloat(_out_f_nom, 3, 1);
                        writeEOL();
                        break;
                
                    case 'T':
                        command_status = COMMAND_SELF_TEST;
                        break;
                    
                    case 'I':
                        _stream->write('#');
                        _stream->print(SERIAL_MONITOR_UPS_INFO);
                        writeEOL();
                        break;

                    case 'S':
                        command_status = COMMAND_SHUTDOWN;
                        break;

                    case 'C':
                        command_status = COMMAND_SHUTDOWN_CANCEL;
                        break;

                    default:
                        _stream->write(_buf);
                        writeEOL();
                        break;

                }
                break;
            default:
                // we assume P protocol as default
                switch(_buf[0]) {
                    case 'T':
                        break;
                    case 'Q':
                        if(_buf[1] == 'S') {
                            ;
                        }
                        else {
                            ;
                        }
                        break;
                    default:
                        _stream->write('N');
                        writeEOL();
                        break;
                }
                break;
        }
    }

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

