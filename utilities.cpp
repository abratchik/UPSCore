#include "utilities.h"

int get_width_modifier(const char* modifier, int *index); 
int get_precision_modifier(const char* modifier, int *index);

/** prints a number to an existing char buffer passed by ref */
void ex_print_number_to_buf(char* _buf, float val, int len, int dec, int base, bool unsgn) {
    memset(_buf, 0x0, len + 2);

    int digits = abs(val * pow(10, dec));
    bool minus = (val < 0) && !unsgn;

    for(int i=0; i < len + (dec?1:0); i++) {
        int index = len - i - (dec?0:1);
        if( digits ||  i <= dec ) {
            if(i != dec || dec == 0 ) {
                int digit = digits % base;
                *(_buf + index) = ( digit < 10? 0x30 : 0x37 ) + digit ;
                digits /= base;
            }
            else {
                *(_buf + index) = '.';
            }
        }
        else if( dec && (i == dec + 1) ) {
            *(_buf + index) = 0x30;
        }
        else if(minus) {
            *(_buf + index) = '-';
            minus = false;
        }
        else {
            *(_buf + index) = i?0x20:0x30;
        }
    }
    
}

void ex_print_number_to_stream(Print* stream, float val, int len, int dec) {
    char * buf = malloc(len+2);
    ex_print_number_to_buf(buf, val, len, dec );
    stream->print(buf);
    free(buf);
}

void ex_print_binary_to_stream(Print* stream,  uint8_t val) {
    for(int i=7; i>=0; i--) {
        stream->write(0x30 + ((val >> i) & 0x01));
    }
}

void ex_print_str_to_stream(Print* stream, const char* str, bool pgm, int fix_len) {
    uint16_t ptr = 0;
    uint16_t len = pgm ?  strlen_P(str): strlen(str);
    while( ptr < len) {
        char c = pgm? (char) pgm_read_byte(&(str[ptr])) : str[ptr];
        stream->write(c);
        ptr++;
    }
    if(fix_len > len) {
        for(int i=len; i < fix_len; i++) {
            stream->write(' ');
        } 
    }
}

void ex_printf_to_stream(Print* stream, const char* fmt, ...) {
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
                    ex_print_binary_to_stream(stream, (int) va_arg(args, int) );
                    break;
                case 'i':   
                    if(wid)
                        ex_print_number_to_stream(stream, (float) va_arg(args, int), wid, 0 );
                    else
                        stream->print((int) va_arg(args, int));
                    break;

                case 'f':
                    if( ( wid > 0 ) && ( prec >= 0 ) ) {
                        ex_print_number_to_stream(stream, (float) va_arg(args, double), wid, prec);
                    }
                    else if( ( prec > 0 ) && ( wid == 0 ) ) {
                        stream->print((float) va_arg(args, double), prec);
                    }
                    else {
                        stream->print((float) va_arg(args, double));
                    }
                    break;
                case 's':
                    ex_print_str_to_stream(stream, (const char *) va_arg(args, const char*),false, wid);
                    break;
                case 'S':
                    ex_print_str_to_stream(stream, (const char *) va_arg(args, const char*),true, wid);
                    break;
                default:
                    stream->write(*(fmt + index));
                    break;    
            }
         }
         else
            stream->write(*(fmt + index));
    }

    va_end(args);

}

int get_width_modifier(const char* modifier, int *index) {
    int value = 0;

    while(*modifier >= '0' && *modifier <= '9') {
        (*index)++;

        value *= 10;
        value += (*modifier - '0');
        modifier++;
    }

    return value;
}

int get_precision_modifier(const char* modifier, int *index) {
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

float ex_parse_float(char* input_buf, int startpos, int len) {
    char * buf = malloc(len);

    memcpy(buf, input_buf + startpos, len);
    float result  = atof(buf);

    free(buf);

    return result;
}